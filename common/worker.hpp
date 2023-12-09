/*
    Coroutines article.
    Class for worker thread. Creates thread and queue for data safe usage.
*/
#pragma once

#include <condition_variable>
#include <coroutine>
#include <queue>
#include <thread>

#include "assingleton.hpp"

enum class WorkerType { Coroutine, Function };
/// @brief Class for thread of all multithread sensetive data modification.
template <WorkerType Type = WorkerType::Coroutine>
class WorkThread : public AsSingleton<WorkThread<Type>> {

  std::jthread m_Worker;
  std::condition_variable m_condVar;
  std::queue<std::conditional_t<Type == WorkerType::Coroutine,
                                std::coroutine_handle<>, std::function<void()>>>
      m_queue;
  std::mutex m_mutex;

public:
  void start() {
    if (m_Worker.joinable())
      return;
    m_Worker = std::jthread{[this]() {
      auto stoken = m_Worker.get_stop_token();
      std::mutex mutex;
      while (true) {
        std::unique_lock lk{mutex};
        m_condVar.wait(lk, [this, stoken] {
          std::lock_guard lg{m_mutex};
          return m_queue.size() || stoken.stop_requested();
        });
        if (stoken.stop_requested())
          break;
        {
          std::lock_guard lg{m_mutex};
          if constexpr (Type == WorkerType::Coroutine)
            m_queue.front().resume();
          else
            m_queue.front()();
          m_queue.pop();
        }
      }
    }};
  }
  void finish() {
    if (m_Worker.joinable()) {
      m_Worker.get_stop_source().request_stop();
      m_condVar.notify_one();
      m_Worker.join();
    }
  }
  void addJob(std::conditional_t<Type == WorkerType::Coroutine,
                                 std::coroutine_handle<>, std::function<void()>>
                  handle) {
    std::lock_guard lg{m_mutex};
    m_queue.emplace(handle);
    m_condVar.notify_one();
  }
  virtual ~WorkThread() { finish(); }
};

template <WorkerType Type> struct WorkerHelper {
  WorkerHelper() { WorkThread<Type>::get().start(); }
  ~WorkerHelper() { WorkThread<Type>::get().finish(); }
};
