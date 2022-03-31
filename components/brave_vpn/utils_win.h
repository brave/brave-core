/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_UTILS_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_UTILS_WIN_H_

#include <string>

#include "base/win/windows_types.h"

namespace brave_vpn {

namespace internal {

enum class CheckConnectionResult {
  CONNECTED,
  CONNECTING,
  CONNECT_FAILED,
  DISCONNECTING,
  DISCONNECTED,
};

void PrintRasError(DWORD error);
std::wstring GetPhonebookPath();

bool CreateEntry(const std::wstring& entry_name,
                 const std::wstring& hostname,
                 const std::wstring& username,
                 const std::wstring& password);
bool RemoveEntry(const std::wstring& entry_name);
bool DisconnectEntry(const std::wstring& entry_name);
bool ConnectEntry(const std::wstring& entry_name);
// Don't cache returned HANDLE. It could be invalidated.
HANDLE GetEventHandleForConnecting();
void CloseEventHandleForConnecting();
HANDLE GetEventHandleForConnectFailed();
void CloseEventHandleForConnectFailed();
HANDLE GetEventHandleForDisconnecting();
void CloseEventHandleForDisconnecting();

CheckConnectionResult CheckConnection(const std::wstring& entry_name);

}  // namespace internal

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_UTILS_WIN_H_
