/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/win/utils.h"

#include <wrl/client.h>

#include <optional>
#include <string>

#include "base/logging.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"

namespace brave_vpn {

HRESULT HRESULTFromLastError() {
  const auto error_code = ::GetLastError();
  return (error_code != NO_ERROR) ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
}

bool IsWindowsServiceRunning(const std::wstring& service_name) {
  auto status = GetWindowsServiceStatus(service_name);
  if (!status.has_value()) {
    return false;
  }
  return status.value() == SERVICE_RUNNING;
}

std::optional<DWORD> GetWindowsServiceStatus(const std::wstring& service_name) {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: " << service_name
            << ", error: " << std::hex << HRESULTFromLastError();
    return std::nullopt;
  }
  ScopedScHandle service(
      ::OpenService(scm.Get(), service_name.c_str(), SERVICE_QUERY_STATUS));

  // Service registered and has not exceeded the number of auto-configured
  // restarts.
  if (!service.IsValid()) {
    return std::nullopt;
  }
  SERVICE_STATUS service_status = {0};
  if (!::QueryServiceStatus(service.Get(), &service_status)) {
    return std::nullopt;
  }
  return service_status.dwCurrentState;
}

bool SetServiceFailureActions(SC_HANDLE service) {
  SC_ACTION failureActions[] = {
      {SC_ACTION_RESTART, 1}, {SC_ACTION_RESTART, 1}, {SC_ACTION_NONE, 1}};
  // The time after which to reset the failure count to zero if there are no
  // failures, in seconds.
  SERVICE_FAILURE_ACTIONS serviceFailureActions = {
      .dwResetPeriod = 0,
      .lpRebootMsg = NULL,
      .lpCommand = NULL,
      .cActions = sizeof(failureActions) / sizeof(SC_ACTION),
      .lpsaActions = failureActions};
  return ChangeServiceConfig2(service, SERVICE_CONFIG_FAILURE_ACTIONS,
                              &serviceFailureActions);
}

}  // namespace brave_vpn
