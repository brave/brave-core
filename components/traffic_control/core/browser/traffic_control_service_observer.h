// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"

namespace traffic_control {

// Observes `TrafficControlService` for changes to traffic control settings.
class TrafficControlServiceObserver : public base::CheckedObserver {
 public:
  // Called when the rules list changes.
  virtual void OnRulesChanged() {}
  // Called when the enabled pref changes.
  virtual void OnEnabledChanged() {}
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_OBSERVER_H_
