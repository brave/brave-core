/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/sequence_checker.h"
#include "base/types/to_address.h"
#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"

namespace brave_vpn::v2 {

BraveVpnServiceImpl::BraveVpnServiceImpl(
    PrefService* local_prefs,
    PrefService* profile_prefs,
    GetSkusServiceCallback skus_service_getter)
    : profile_prefs_(CHECK_DEREF(profile_prefs)),
      skus_client_(
          std::make_unique<SkusServiceClient>(std::move(skus_service_getter))),
      connection_state_(mojom::ConnectionState::DISCONNECTED) {
  DCHECK(IsBraveVPNFeatureEnabled());
  purchased_state_manager_ = std::make_unique<PurchasedStateManager>(
      local_prefs, skus_client_.get(),
      base::BindRepeating(&BraveVpnServiceImpl::OnPurchasedStateChanged,
                          base::Unretained(this)));
}

BraveVpnServiceImpl::~BraveVpnServiceImpl() = default;

bool BraveVpnServiceImpl::IsBraveVPNEnabled() const {
  return ::brave_vpn::IsBraveVPNEnabled(base::to_address(profile_prefs_));
}

bool BraveVpnServiceImpl::IsPurchased() const {
  if (!purchased_state_manager_) {
    return false;
  }
  return purchased_state_manager_->IsPurchased();
}

void BraveVpnServiceImpl::ReloadPurchasedState() {
  if (purchased_state_manager_) {
    purchased_state_manager_->Reload();
  }
}

std::string BraveVpnServiceImpl::GetCurrentEnvironment() const {
  if (!purchased_state_manager_) {
    return {};
  }
  return purchased_state_manager_->GetCurrentEnvironment();
}

void BraveVpnServiceImpl::GetPurchasedState(
    GetPurchasedStateCallback callback) {
  if (purchased_state_manager_) {
    std::move(callback).Run(purchased_state_manager_->GetInfo().Clone());
    return;
  }
  std::move(callback).Run(mojom::PurchasedInfo::New(
      mojom::PurchasedState::NOT_PURCHASED, std::nullopt));
}

void BraveVpnServiceImpl::LoadPurchasedState(const std::string& domain) {
  if (purchased_state_manager_) {
    purchased_state_manager_->Load(domain);
  }
}

void BraveVpnServiceImpl::GetAllRegions(GetAllRegionsCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run({});
}

void BraveVpnServiceImpl::Shutdown() {
  purchased_state_manager_.reset();
  skus_client_->Reset();
  BraveVpnService::Shutdown();
}

void BraveVpnServiceImpl::OnPurchasedStateChanged(
    mojom::PurchasedState state,
    std::optional<std::string> description) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  NotifyPurchasedStateChanged(state, description);

  // TODO: If purchased state changed to PURCHASED on desktop, we can attempt to
  // install VPN apps, connect to the agent, etc. We also need to make sure
  // agent gets connected and fetches the region data - BEFORE we actually send
  // a notification to the UI.
}

}  // namespace brave_vpn::v2
