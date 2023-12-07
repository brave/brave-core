/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#include "brave/installer/win/util/brave_vpn_helper_utils.h"
#endif

#include "src/chrome/elevation_service/elevator.cc"

namespace elevation_service {

HRESULT Elevator::InstallVPNServices() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (!brave_vpn::IsBraveVPNHelperServiceInstalled()) {
    HRESULT hr = brave_vpn::InstallBraveVPNHelperServiceImpersonated();
    if (FAILED(hr)) {
      return hr;
    }
  }

  if (!brave_vpn::wireguard::IsWireguardServiceInstalled()) {
    HRESULT hr = brave_vpn::InstallBraveWireGuardServiceImpersonated();
    if (FAILED(hr)) {
      return hr;
    }
  }
#endif
  return S_OK;
}

}  // namespace elevation_service
