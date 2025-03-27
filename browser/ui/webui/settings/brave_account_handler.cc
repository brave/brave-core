/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account_handler.h"

#include <utility>

#include "components/password_manager/core/browser/ui/weak_check_utility.h"

namespace brave_account {
BraveAccountHandler::BraveAccountHandler(
    mojo::PendingReceiver<mojom::BraveAccountHandler> handler)
    : handler_(this, std::move(handler)) {}

BraveAccountHandler::~BraveAccountHandler() = default;

void BraveAccountHandler::GetPasswordStrength(
    const std::string& password,
    mojom::BraveAccountHandler::GetPasswordStrengthCallback callback) {
  std::move(callback).Run(password_manager::GetPasswordStrength(password));
}
}  // namespace brave_account
