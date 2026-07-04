/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/notimplemented.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn::v2 {

PurchasedStateManager::PurchasedStateManager(
    PrefService* local_prefs,
    PurchasedStateChangedCallback callback)
    : local_prefs_(CHECK_DEREF(local_prefs)),
      purchased_state_changed_callback_(std::move(callback)) {
  CHECK(purchased_state_changed_callback_);
}

PurchasedStateManager::~PurchasedStateManager() = default;

void PurchasedStateManager::Reload() {
  Load(skus::GetDomain(skus::GetVpnProductPrefix(), GetCurrentEnvironment()));
}

void PurchasedStateManager::Load(const std::string& domain) {
  NOTIMPLEMENTED();
}

mojom::PurchasedInfo PurchasedStateManager::GetInfo() const {
  return purchased_state_.value_or(
      mojom::PurchasedInfo(mojom::PurchasedState::NOT_PURCHASED, std::nullopt));
}

bool PurchasedStateManager::IsPurchased() const {
  return GetInfo().state == mojom::PurchasedState::PURCHASED;
}

std::string PurchasedStateManager::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void PurchasedStateManager::SetPurchasedState(
    const std::string& env,
    mojom::PurchasedState state,
    std::optional<std::string> description) {
  NOTIMPLEMENTED();
}

}  // namespace brave_vpn::v2
