/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/elevation_service/elevator.h"

#include <windows.h>

#include <intsafe.h>
#include <winerror.h>

#include "base/path_service.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/install_utils.h"
#include "brave/browser/brave_vpn/win/wireguard_utils_win.h"
#endif
#endif

#include "src/chrome/elevation_service/elevator.cc"

namespace elevation_service {

HRESULT Elevator::InstallVPNServices() {
  // Perform a trusted source check.
  // This ensures the caller is an executable in `%PROGRAMFILES%`.
  // For more info, see https://github.com/brave/brave-core/pull/24900
  HRESULT hr = ::CoImpersonateClient();
  if (FAILED(hr)) {
    return hr;
  }

  {
    absl::Cleanup revert_to_self = [] { ::CoRevertToSelf(); };

    const auto process = GetCallingProcess();
    if (!process.IsValid()) {
      return kErrorCouldNotObtainCallingProcess;
    }
    const auto validation_data = GenerateValidationData(
        ProtectionLevel::PROTECTION_PATH_VALIDATION, process);
    if (!validation_data.has_value()) {
      return validation_data.error();
    }
    const auto data = std::vector<uint8_t>(validation_data->cbegin(),
                                           validation_data->cend());

    // Note: Validation should always be done using caller impersonation token.
    std::string log_message;
    HRESULT validation_result = ValidateData(process, data, &log_message);
    if (FAILED(validation_result)) {
      return validation_result;
    }
  }
  // End of trusted source check

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (!brave_vpn::IsBraveVPNHelperServiceInstalled()) {
    auto success = brave_vpn::InstallBraveVPNHelperService(
        base::PathService::CheckedGet(base::DIR_ASSETS));
    if (!success) {
      return E_FAIL;
    }
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  if (!brave_vpn::wireguard::IsWireguardServiceInstalled()) {
    auto success = brave_vpn::InstallBraveWireguardService(
        base::PathService::CheckedGet(base::DIR_ASSETS));
    if (!success) {
      return E_FAIL;
    }
  }
#endif
#endif
  return S_OK;
}

}  // namespace elevation_service
