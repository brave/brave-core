// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace traffic_control {

class TrafficControlServiceObserver : public base::CheckedObserver {
 public:
  ~TrafficControlServiceObserver() override = default;
  virtual void OnRulesChanged() {}
  virtual void OnEnabledChanged() {}
};

// Profile-scoped service for Traffic Control rules. Pref I/O is confined to
// prefs.{h,cc}; callers use this typed API. Mutations always rewrite the full
// syncable list (last-writer-wins). List order is evaluation / display order
// (first match wins).
class TrafficControlService : public KeyedService {
 public:
  explicit TrafficControlService(PrefService* prefs);
  ~TrafficControlService() override;

  TrafficControlService(const TrafficControlService&) = delete;
  TrafficControlService& operator=(const TrafficControlService&) = delete;

  void AddObserver(TrafficControlServiceObserver* observer);
  void RemoveObserver(TrafficControlServiceObserver* observer);

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  std::vector<mojom::TrafficRulePtr> GetRules() const;

  // CRUD always loads the full list, mutates, and writes it back.
  std::optional<mojom::RuleOperationError> AddRule(mojom::TrafficRulePtr rule);
  std::optional<mojom::RuleOperationError> UpdateRule(
      mojom::TrafficRulePtr rule);
  std::optional<mojom::RuleOperationError> RemoveRule(std::string_view id);

 private:
  void OnRulesPrefChanged();
  void OnEnabledPrefChanged();

  static std::optional<mojom::RuleOperationError> ValidateRule(
      const mojom::TrafficRulePtr& rule,
      bool require_empty_id);

  raw_ptr<PrefService> prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<TrafficControlServiceObserver> observers_;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
