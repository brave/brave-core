// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_VPN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_VPN_UTILS_H_

#include <windows.h>
#include <string>

#include "base/types/expected.h"
#include "base/win/scoped_handle.h"

namespace brave_vpn {

// Helper for methods which perform system operations which may fail. The
// failure reason is returned as an HRESULT.
// TODO(crbug.com/1369769): Remove the following warning once resolved in
// base. NOTE: When ValueT is an integral type, base::expected's implicit ctors
// are ambiguous. To return an error in this case it must be wrapped in a
// base::unexpected(error);
template <typename ValueT>
using HResultOr = base::expected<ValueT, HRESULT>;

class ScHandleTraits {
 public:
  using Handle = SC_HANDLE;

  ScHandleTraits() = delete;
  ScHandleTraits(const ScHandleTraits&) = delete;
  ScHandleTraits& operator=(const ScHandleTraits&) = delete;

  static bool CloseHandle(SC_HANDLE handle) {
    return ::CloseServiceHandle(handle) != FALSE;
  }

  static bool IsHandleValid(SC_HANDLE handle) { return handle != nullptr; }

  static SC_HANDLE NullHandle() { return nullptr; }
};

using ScopedScHandle =
    base::win::GenericScopedHandle<ScHandleTraits,
                                   base::win::DummyVerifierTraits>;

// Register and setup DNS filters layer to the system, if the layer is already
// registered reuses existing.
bool AddWpmFilters(HANDLE engine_handle, const std::string& name);
// Opens a session to a filter engine.
HANDLE OpenWpmSession();
// Closes a session to a filter engine.
bool CloseWpmSession(HANDLE engine);
// Subscribes for RAS connection notification of any os vpn entry.
bool SubscribeRasConnectionNotification(HANDLE event_handle);
// Configure VPN Service autorestart.
bool ConfigureServiceAutoRestart(const std::wstring& service_name,
                                 const std::wstring& brave_vpn_entry);
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_VPN_UTILS_H_
