/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/elevation_service/elevator.h"

#include <windows.h>
#include <winerror.h>

#include <intsafe.h>

#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/install_utils.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#endif

#include "src/chrome/elevation_service/elevator.cc"

namespace elevation_service {

HRESULT Elevator::InstallVPNServices() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (!brave_vpn::IsBraveVPNHelperServiceInstalled()) {
    auto success = brave_vpn::InstallBraveVPNHelperService();
    if (!success) {
      return E_FAIL;
    }
  }

  if (!brave_vpn::wireguard::IsWireguardServiceInstalled()) {
    auto success = brave_vpn::InstallBraveWireguardService();
    if (!success) {
      return E_FAIL;
    }
  }
#endif
  return S_OK;
}

}  // namespace elevation_service
