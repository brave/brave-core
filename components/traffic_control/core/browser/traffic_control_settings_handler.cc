// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_settings_handler.h"

#include <utility>

namespace traffic_control {

TrafficControlSettingsHandler::TrafficControlSettingsHandler(
    TrafficControlService* service)
    : service_(service) {
  CHECK(service_);
  service_->AddObserver(this);
}

TrafficControlSettingsHandler::~TrafficControlSettingsHandler() {
  service_->RemoveObserver(this);
}

void TrafficControlSettingsHandler::BindUI(
    mojo::PendingRemote<mojom::TrafficControlSettingsUI> ui) {
  DCHECK(!ui_);
  ui_.Bind(std::move(ui));
}

void TrafficControlSettingsHandler::GetRules(GetRulesCallback callback) {
  std::move(callback).Run(service_->GetRules());
}

void TrafficControlSettingsHandler::AddRule(mojom::TrafficRulePtr rule,
                                            AddRuleCallback callback) {
  std::move(callback).Run(service_->AddRule(std::move(rule)));
}

void TrafficControlSettingsHandler::UpdateRule(mojom::TrafficRulePtr rule,
                                               UpdateRuleCallback callback) {
  std::move(callback).Run(service_->UpdateRule(std::move(rule)));
}

void TrafficControlSettingsHandler::RemoveRule(const std::string& id,
                                               RemoveRuleCallback callback) {
  std::move(callback).Run(service_->RemoveRule(id));
}

void TrafficControlSettingsHandler::OnRulesChanged() {
  if (ui_) {
    ui_->OnRulesChanged(service_->GetRules());
  }
}

}  // namespace traffic_control
