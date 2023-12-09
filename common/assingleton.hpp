
/*
    Coroutines article.
    Class for making inherited class as singleton.
*/


template <class T> class AsSingleton {

  inline static T m_Instance;

public:
  AsSingleton() = default;
  AsSingleton(const AsSingleton &) = delete;
  AsSingleton(AsSingleton &&) = delete;
  AsSingleton &operator=(const AsSingleton &) = delete;
  AsSingleton &operator=(AsSingleton &&) = delete;

  static T &get() { return m_Instance; }
};
