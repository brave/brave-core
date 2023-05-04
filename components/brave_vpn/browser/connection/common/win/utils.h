/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_COMMON_WIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_COMMON_WIN_UTILS_H_

#include "base/win/windows_types.h"

namespace brave_vpn {
HRESULT HRESULTFromLastError();
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_COMMON_WIN_UTILS_H_
