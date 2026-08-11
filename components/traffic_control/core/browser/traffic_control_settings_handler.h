// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SETTINGS_HANDLER_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SETTINGS_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace traffic_control {

// Handles traffic control rule management operations from the settings UI.
class TrafficControlSettingsHandler
    : public mojom::TrafficControlSettingsHandler {
 public:
  explicit TrafficControlSettingsHandler(PrefService* prefs);
  ~TrafficControlSettingsHandler() override;

  TrafficControlSettingsHandler(const TrafficControlSettingsHandler&) = delete;
  TrafficControlSettingsHandler& operator=(
      const TrafficControlSettingsHandler&) = delete;

  // mojom::TrafficControlSettingsHandler:
  void BindUI(mojo::PendingRemote<mojom::TrafficControlSettingsUI> ui) override;
  void GetRules(GetRulesCallback callback) override;
  void AddRule(mojom::TrafficRulePtr rule, AddRuleCallback callback) override;
  void UpdateRule(mojom::TrafficRulePtr rule,
                  UpdateRuleCallback callback) override;
  void RemoveRule(const std::string& id, RemoveRuleCallback callback) override;

 private:
  void OnRulesPrefChanged();

  mojo::Remote<mojom::TrafficControlSettingsUI> ui_;
  raw_ptr<PrefService> prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SETTINGS_HANDLER_H_
