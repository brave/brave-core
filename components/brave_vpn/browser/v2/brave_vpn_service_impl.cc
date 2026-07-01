/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include "base/check_deref.h"
#include "base/notimplemented.h"
#include "base/types/to_address.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {
namespace v2 {

BraveVpnServiceImpl::BraveVpnServiceImpl(PrefService* local_prefs,
                                         PrefService* profile_prefs)
    : local_prefs_(CHECK_DEREF(local_prefs)),
      profile_prefs_(CHECK_DEREF(profile_prefs)),
      connection_state_(mojom::ConnectionState::DISCONNECTED),
      purchased_state_(mojom::PurchasedState::NOT_PURCHASED) {
  DCHECK(IsBraveVPNFeatureEnabled());
}

BraveVpnServiceImpl::~BraveVpnServiceImpl() = default;

bool BraveVpnServiceImpl::IsBraveVPNEnabled() const {
  return ::brave_vpn::IsBraveVPNEnabled(base::to_address(profile_prefs_));
}

bool BraveVpnServiceImpl::IsPurchased() const {
  NOTIMPLEMENTED();
  return purchased_state_ == mojom::PurchasedState::PURCHASED;
}

void BraveVpnServiceImpl::ReloadPurchasedState() {
  NOTIMPLEMENTED();
}

std::string BraveVpnServiceImpl::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVpnServiceImpl::GetPurchasedState(
    GetPurchasedStateCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      mojom::PurchasedInfo::New(purchased_state_, std::nullopt));
}

void BraveVpnServiceImpl::LoadPurchasedState(const std::string& domain) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetAllRegions(GetAllRegionsCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run({});
}

void BraveVpnServiceImpl::Shutdown() {
  BraveVpnService::Shutdown();
}

}  // namespace v2
}  // namespace brave_vpn
