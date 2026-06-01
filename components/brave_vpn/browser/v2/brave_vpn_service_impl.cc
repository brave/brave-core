/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include "base/notimplemented.h"
#include "brave/components/skus/browser/skus_utils.h"

namespace brave_vpn {
namespace v2 {

BraveVpnServiceImpl::BraveVpnServiceImpl()
    : connection_state_(mojom::ConnectionState::DISCONNECTED),
      purchased_state_(mojom::PurchasedState::NOT_PURCHASED) {}

BraveVpnServiceImpl::~BraveVpnServiceImpl() = default;

bool BraveVpnServiceImpl::IsBraveVPNEnabled() const {
  NOTIMPLEMENTED();
  return false;
}

bool BraveVpnServiceImpl::IsPurchased() const {
  NOTIMPLEMENTED();
  return purchased_state_ == mojom::PurchasedState::PURCHASED;
}

void BraveVpnServiceImpl::ReloadPurchasedState() {
  NOTIMPLEMENTED();
}

std::string BraveVpnServiceImpl::GetCurrentEnvironment() const {
  NOTIMPLEMENTED();
  return skus::kEnvProduction;
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
