#pragma once
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>

namespace aseandroid {
// Portable FIFO for LAF's Android event queue, retaining os::Event upstream.
template<class T>
class EventQueueCore {
public:
  EventQueueCore() = default;
  EventQueueCore(const EventQueueCore&) = delete;
  EventQueueCore& operator=(const EventQueueCore&) = delete;
  void push(const T& event) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_) return;
      queue_.push_back(event);
    }
    condition_.notify_one();
  }
  // Negative timeout waits indefinitely; zero polls without waiting.
  bool pop(T& event, double timeoutSeconds) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto ready = [this] { return closed_ || !queue_.empty(); };
    if (timeoutSeconds < 0)
      condition_.wait(lock, ready);
    else if (timeoutSeconds > 0)
      condition_.wait_for(lock, std::chrono::duration<double>(timeoutSeconds), ready);
    if (queue_.empty()) return false;
    event = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
  }
  void close() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
      queue_.clear();
    }
    condition_.notify_all();
  }
private:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<T> queue_;
  bool closed_ = false;
};
} // namespace aseandroid
