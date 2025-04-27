#include <condition_variable>

template <typename T>
class ThreadSafeVar {
public:
    ThreadSafeVar() : value() {}
    ThreadSafeVar(const T value) : value(value) {}

    T get() const {
      std::lock_guard<std::mutex> lock(m);
      return value;
    }

    void set(const T newValue){
      std::lock_guard<std::mutex> lock(m);
      value = newValue;
      cv.notify_all();
    }

    void waitForValue(T new_value) {
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [&]() { return value == new_value; });
    }

private:
    T value;
    mutable std::mutex m;
    std::condition_variable cv;
};

