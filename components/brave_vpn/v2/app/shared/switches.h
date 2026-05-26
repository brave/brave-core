/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_SHARED_SWITCHES_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_SHARED_SWITCHES_H_

namespace brave_vpn {
namespace v2 {
namespace switches {

inline constexpr char kVpnAppLogFile[] = "log-file";
inline constexpr char kVpnAppProcessType[] = "type";
inline constexpr char kVpnAppUserDataDir[] = "user-data-dir";
inline constexpr char kVpnAppCrashOnStartup[] = "crash-on-startup";

}  // namespace switches
}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_SHARED_SWITCHES_H_
