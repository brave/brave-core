// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_vpn/win/vpn_utils_win.h"

#include <wrl/client.h>

#include "base/logging.h"
#include "base/win/com_init_util.h"
#include "chrome/elevation_service/elevation_service_idl.h"
#include "chrome/install_static/install_util.h"

namespace brave_vpn {

bool InstallVpnSystemServices() {
  base::win::AssertComInitialized();

  Microsoft::WRL::ComPtr<IElevator> elevator;
  HRESULT hr = CoCreateInstance(
      install_static::GetElevatorClsid(), nullptr, CLSCTX_LOCAL_SERVER,
      install_static::GetElevatorIid(), IID_PPV_ARGS_Helper(&elevator));
  if (FAILED(hr)) {
    VLOG(1) << "CoCreateInstance returned: 0x" << std::hex << hr;
    return false;
  }

  hr = CoSetProxyBlanket(
      elevator.Get(), RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
      COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
      RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_DYNAMIC_CLOAKING);
  if (FAILED(hr)) {
    VLOG(1) << "CoSetProxyBlanket returned: 0x" << std::hex << hr;
    return false;
  }

  hr = elevator->InstallVPNServices();
  if (FAILED(hr)) {
    VLOG(1) << "InstallVPNServices returned: 0x" << std::hex << hr;
    return false;
  }

  VLOG(1) << "InstallVPNServices: SUCCESS";
  return true;
}

}  // namespace brave_vpn
