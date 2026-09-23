// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/types/optional_ref.h"
#include "brave/components/traffic_control/core/browser/traffic_rule_matcher.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class GURL;
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

  // Returns the first matching enabled rule for |url|, or empty if the feature
  // pref is off or nothing matches. The reference is valid until the next
  // matcher rebuild.
  base::optional_ref<const mojom::TrafficRule> FindMatchingRule(
      const GURL& url) const;

  // KeyedService:
  void Shutdown() override;

 private:
  void RebuildMatcher();

  raw_ptr<PrefService> prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  TrafficRuleMatcher matcher_;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SERVICE_H_
