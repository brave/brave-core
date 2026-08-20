// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace traffic_control {

// Profile-scoped service for Traffic Control. Pref I/O is confined to
// prefs.{h,cc}; settings UI CRUD goes through TrafficControlSettingsHandler.
class TrafficControlService : public KeyedService {
 public:
  explicit TrafficControlService(PrefService* prefs);
  ~TrafficControlService() override;

  TrafficControlService(const TrafficControlService&) = delete;
  TrafficControlService& operator=(const TrafficControlService&) = delete;

  bool IsEnabled() const;

  std::vector<mojom::TrafficRulePtr> GetRules() const;

 private:
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
