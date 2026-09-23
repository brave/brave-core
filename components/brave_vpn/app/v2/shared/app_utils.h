/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_APP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_APP_UTILS_H_

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "build/build_config.h"

namespace brave_vpn::v2::app_utils {

void InitLogging(const base::CommandLine& command_line);

// Returns the per-product user data directory, formed by appending the Brave
// company name and `product_name` to a platform-specific base path.
// `product_name` must be non-empty, otherwise it will be a CHECK failure.
//
// The base path depends on the platform and on `is_privileged_process`. When
// `is_privileged_process` is true, a machine-wide location is used (Windows:
// common app data; macOS: /Library/Application Support; Linux: /var/lib).
// When false, a per-user location is used (Windows: local app data; macOS:
// per-user app data; Linux: XDG config home).
base::FilePath GetUserDataDir(const std::string& product_name,
                              bool is_privileged_process);

#if BUILDFLAG(IS_MAC)
// Resolves the framework path for a process whose executable lives in
// `exe_dir`. When `am_i_bundled` is true the agent is nested inside the
// framework, so the enclosing ".framework" ancestor of `exe_dir` is returned.
// When false (a bare binary run from the build output dir) the sibling
// ".framework" directory inside `exe_dir` is returned. Returns an empty path
// if no framework is found.
base::FilePath ResolveFrameworkPath(const base::FilePath& exe_dir,
                                    bool am_i_bundled);

// Points base's framework bundle path at the enclosing/sibling ".framework"
// so crashpad can locate chrome_crashpad_handler. Must be called before crash
// reporting is initialized.
void ConfigureFrameworkBundlePath();
#endif

}  // namespace brave_vpn::v2::app_utils

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_SHARED_APP_UTILS_H_
