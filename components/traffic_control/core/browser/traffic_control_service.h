// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service_observer.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace traffic_control {

class TrafficControlService : public KeyedService {
 public:
  explicit TrafficControlService(PrefService* prefs);
  ~TrafficControlService() override;

  TrafficControlService(const TrafficControlService&) = delete;
  TrafficControlService& operator=(const TrafficControlService&) = delete;

  void Shutdown() override;

  void AddObserver(TrafficControlServiceObserver* observer);
  void RemoveObserver(TrafficControlServiceObserver* observer);

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  std::vector<mojom::TrafficRulePtr> GetRules() const;

 private:
  void OnRulesPrefChanged();
  void OnEnabledPrefChanged();

  raw_ptr<PrefService> prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<TrafficControlServiceObserver> observers_;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
