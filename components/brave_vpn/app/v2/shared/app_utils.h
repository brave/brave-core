/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_APP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_APP_UTILS_H_

#include "base/command_line.h"

namespace brave_vpn {
namespace v2 {
namespace app_utils {

void InitLogging(const base::CommandLine& command_line);

}  // namespace app_utils
}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_APP_UTILS_H_
