/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_SHARED_APP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_SHARED_APP_UTILS_H_

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"

namespace brave_vpn {
namespace v2 {

void AppInitLogging(const base::CommandLine& command_line);
base::FilePath AppGetPrivilegedUserDataDir(const std::string& product_name);
base::FilePath AppGetUnprivilegedUserDataDir(const std::string& product_name);

}  // namespace v2
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_V2_APP_SHARED_APP_UTILS_H_
