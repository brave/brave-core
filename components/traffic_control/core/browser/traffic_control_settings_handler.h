// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SETTINGS_HANDLER_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SETTINGS_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace traffic_control {

// Mojo settings handler. Delegates CRUD to TrafficControlService (typed API);
// does not write PrefService rule prefs directly. The enabled toggle is bound
// via chrome.settingsPrivate (pref-key) instead of mojo.
class TrafficControlSettingsHandler
    : public mojom::TrafficControlSettingsHandler,
      public TrafficControlServiceObserver {
 public:
  explicit TrafficControlSettingsHandler(TrafficControlService* service);
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

  // TrafficControlServiceObserver:
  void OnRulesChanged() override;

 private:
  raw_ptr<TrafficControlService> service_ = nullptr;
  mojo::Remote<mojom::TrafficControlSettingsUI> ui_;
};

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_TRAFFIC_CONTROL_SETTINGS_HANDLER_H_
