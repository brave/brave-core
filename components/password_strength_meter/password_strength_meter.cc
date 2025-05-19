/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/password_strength_meter/password_strength_meter.h"

#include <utility>

#include "brave/components/password_strength_meter/zxcvbn_data_component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/password_manager/core/browser/ui/weak_check_utility.h"

namespace password_strength_meter {

void RegisterZxcvbnDataComponent(
    component_updater::ComponentUpdateService* cus) {
  component_updater::RegisterZxcvbnDataComponent(cus);
}

PasswordStrengthMeterHandler::PasswordStrengthMeterHandler(
    mojo::PendingReceiver<mojom::PasswordStrengthMeterHandler> handler)
    : handler_(this, std::move(handler)) {}

PasswordStrengthMeterHandler::~PasswordStrengthMeterHandler() = default;

void PasswordStrengthMeterHandler::GetPasswordStrength(
    const std::string& password,
    mojom::PasswordStrengthMeterHandler::GetPasswordStrengthCallback callback) {
  std::move(callback).Run(password_manager::GetPasswordStrength(password));
}

}  // namespace password_strength_meter
