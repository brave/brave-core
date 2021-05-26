/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_UTILS_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_UTILS_WIN_H_

#include <windows.h>

namespace brave_vpn {

void PrintRasError(DWORD error);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_UTILS_WIN_H_
