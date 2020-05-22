// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EVENT_MONITOR_ANDROID_H_
#define UI_VIEWS_EVENT_MONITOR_ANDROID_H_

#include "base/macros.h"
#include "ui/views/event_monitor.h"

namespace ui {
class EventTarget;
}

namespace views {

// Observes events by installing a pre-target handler on the ui::EventTarget.
class EventMonitorAndroid : public EventMonitor {
 public:
  EventMonitorAndroid(ui::EventObserver* event_observer,
                   ui::EventTarget* event_target,
                   const std::set<ui::EventType>& types);
  ~EventMonitorAndroid() override;

  // EventMonitor:
  gfx::Point GetLastMouseLocation() override;

 protected:

 private:

  DISALLOW_COPY_AND_ASSIGN(EventMonitorAndroid);
};

}  // namespace views

#endif  // UI_VIEWS_EVENT_MONITOR_Android_H_
