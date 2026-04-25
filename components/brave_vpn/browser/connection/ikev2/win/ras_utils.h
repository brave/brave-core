/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_RAS_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_RAS_UTILS_H_

#include <string>

#include "base/win/windows_types.h"

namespace brave_vpn {

class BraveVPNConnectionInfo;

namespace ras {

enum class CheckConnectionResult {
  CONNECTED,
  CONNECTING,
  CONNECT_FAILED,
  DISCONNECTING,
  DISCONNECTED,
};

struct RasOperationResult {
  bool success;
  // If not success, store user friendly error description.
  std::string error_description;
};

// Returns human readable error description.
std::string GetRasErrorMessage(DWORD error);
std::wstring GetPhonebookPath(const std::wstring& entry_name,
                              std::string* error);

RasOperationResult CreateEntry(const BraveVPNConnectionInfo& info);
RasOperationResult RemoveEntry(const std::wstring& entry_name);
RasOperationResult DisconnectEntry(const std::wstring& entry_name);
RasOperationResult ConnectEntry(const std::wstring& entry_name);
CheckConnectionResult CheckConnection(const std::wstring& entry_name);

}  // namespace ras

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_WIN_RAS_UTILS_H_
