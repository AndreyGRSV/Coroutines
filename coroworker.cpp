/*
    Coroutines article.
    Sample of usage of coroutines

*/
#include <coroutine>
#include <future>
#include <iostream>

#include "worker.hpp"

template <typename T> struct returnType {
  struct promise_type {
    auto get_return_object() {
      return returnType{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_value(T value) {
      m_value = value;
      m_promise.set_value(value);
    }
    T get_direct() { return m_value; }
    T get_async() { return m_promise.get_future().get(); }

  private:
    T m_value;
    std::promise<T> m_promise;
  };

  returnType(std::coroutine_handle<promise_type> handle) : m_handle(handle) {}

  std::coroutine_handle<> handle() const { return m_handle; }

  T get() const { return m_handle.promise().get_direct(); }
  T get_async() const { return m_handle.promise().get_async(); }

private:
  std::coroutine_handle<promise_type> m_handle;
};

struct awaitExecution {
  bool await_ready() const { return false; }
  void await_suspend(std::coroutine_handle<> handle) const {
    WorkThread<WorkerType::Coroutine>::get().addJob(handle);
  }
  void await_resume() const {}
};

class SomeData {
  uint64_t m_id = 0;
  const char *m_name = nullptr;
  uint64_t m_value = 0;

public:
  SomeData() = default;
  ~SomeData() = default;
  SomeData(const SomeData &) = default;
  SomeData &operator=(const SomeData &) = delete;
  SomeData(SomeData &&) = delete;
  SomeData &operator=(SomeData &&) = delete;
  SomeData(uint64_t id, const char *name) : m_id{id}, m_name(name) {}
  uint64_t id() const { return m_id; }
  std::string name() const { return m_name; }
};

// Fake operation
enum class OpType { Update, Insert };

// Normal function
bool updateData(const SomeData &data, OpType type) {
  // Data is not correct
  if (data.id() < 3)
    return false;

  // Do something
  std::cout << "data id:" << data.id() << ", data name:" << data.name()
            << std::endl;
  return true;
}

// Result of updateDataCoro coroutine
enum class CoExecutionResult { None, ImmediateError, Success, Error };

// Coroutine
returnType<CoExecutionResult> updateDataCoro(const SomeData &data,
                                             OpType type) {
  // Data is not correct
  if (data.id() < 3)
    co_return CoExecutionResult::ImmediateError;

  // Store data before exit from thread
  SomeData store_data{data};
  co_await awaitExecution{};
  // Do something
  std::cout << "data id:" << store_data.id()
            << ", data name:" << store_data.name() << std::endl;
  if (store_data.id() == 4)
    co_return CoExecutionResult::Error;
  co_return CoExecutionResult::Success;
}

int main() {

  // -------------- Coroutine
  const WorkerHelper<WorkerType::Coroutine> helperCoroutines;

  // Add coroutine asynchronously
  for (int i = 0; i < 10; i++) {

    auto handle2 = std::async(std::launch::async, [=]() {
      std::cout << "start async 1. thread id:" << std::this_thread::get_id()
                << '\n';

      // Coroutine placed to worker
      auto result =
          updateDataCoro({static_cast<uint64_t>(i), "Name"}, OpType::Update);

      // Get immediate result if present
      if (result.get() == CoExecutionResult::ImmediateError) {
        std::cout << i
                  << ". updateDataCoro immediate result is "
                     "CoExecutionResult::ImmediateError"
                  << '\n';
      } else {
        std::cout << i << ". updateDataCoro no error" << '\n';
      }

      // Wait async result
      switch (result.get_async()) {
      case CoExecutionResult::Success:
        std::cout << i << ". updateDataCoro CoExecutionResult::Success" << '\n';
        break;
      case CoExecutionResult::Error:
        std::cout << i << ". updateDataCoro CoExecutionResult::Error" << '\n';
        break;
      }
    });
  }

  // ---------------- Normal function
  const WorkerHelper<WorkerType::Function> helperFunctions1;
  using FWorker = WorkThread<WorkerType::Function>;

  // Add functions asynchronously
  for (int i = 0; i < 10; i++) {

    std::async(std::launch::async, [=] {
      std::cout << "start async 2. thread id:" << std::this_thread::get_id()
                << '\n';

      std::promise<bool> promise;

      SomeData data{static_cast<const uint64_t>(i), "Name"};

      FWorker::get().addJob([=, &promise]() {
        promise.set_value(updateData(data, OpType::Insert));
      });
      std::cout << i << ". future get:" << std::boolalpha
                << promise.get_future().get() << '\n';
    });
  }

  return 0;
}