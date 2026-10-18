#pragma once
// Native event queue for original laf/os::Event, not a parallel UI model.
#include "os/event.h"
#include "os/event_queue.h"
#include "os/android/event_queue_core.h"
namespace os {
class EventQueueAndroid final : public EventQueue {
public:
  ~EventQueueAndroid() override { events_.close(); }
  void queueEvent(const Event& ev) override { events_.push(ev); }
  void getEvent(Event& ev, double timeout) override {
    if (!events_.pop(ev,timeout)) ev=Event();
  }
  void clearEvents() override { events_.clear(); }
private:
  aseandroid::EventQueueCore<Event> events_;
};
using EventQueueImpl=EventQueueAndroid;
} // namespace os
