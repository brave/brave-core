/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/test/fake_brave_vpn_service.h"

#include "brave/components/skus/browser/skus_utils.h"

namespace brave_vpn {

bool FakeBraveVpnService::IsBraveVPNEnabled() const {
  return true;
}

bool FakeBraveVpnService::IsPurchased() const {
  return true;
}

std::string FakeBraveVpnService::GetCurrentEnvironment() const {
  return skus::kEnvProduction;
}

#if !BUILDFLAG(IS_ANDROID)

bool FakeBraveVpnService::IsConnected() const {
  return false;
}

mojom::ConnectionState FakeBraveVpnService::GetConnectionState() const {
  return mojom::ConnectionState::DISCONNECTED;
}

std::string FakeBraveVpnService::GetLastConnectionError() const {
  return {};
}

#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_vpn
