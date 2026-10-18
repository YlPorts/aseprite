#include "event_queue_core.h"
#include <cassert>
#include <thread>
#include <atomic>
#include <iostream>
using aseandroid::EventQueueCore;
int main() {
  EventQueueCore<int> queue;
  int out=-1;
  assert(!queue.pop(out,0));
  queue.push(11); queue.push(12);
  assert(queue.pop(out,0) && out==11);
  assert(queue.pop(out,0) && out==12);
  assert(!queue.pop(out,.002));
  std::atomic<bool> got{false};
  std::thread reader([&] { int x=0; got=(queue.pop(x,-1) && x==42); });
  queue.push(42);
  reader.join();
  assert(got.load());
  queue.push(7);
  queue.clear();
  assert(!queue.pop(out,0));
  std::atomic<bool> exited{false};
  std::thread blocked([&] { int x=0; exited=!queue.pop(x,-1); });
  queue.close();
  blocked.join();
  assert(exited && !queue.pop(out,0));
  std::cout << "PASS: FIFO, polling, timeout, wakeup, clear, shutdown\n";
}
