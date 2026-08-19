/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/app_shim/app_shim_controller.h"

#include <string_view>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "brave/app_shim/test_launch_url.h"
#include "url/gurl.h"

namespace {

// Upstream launches the app shim in tests via LaunchServices. This leaks
// processes and eventually clogs up sandboxd - see the top-level comment in
// app_shim_launch.mm. To work around this, we instead launch the app shim via
// fork/exec. This has the following limitation: A shim spawned with
// LaunchServices receives its launch URLs through -application:openURLs:. A
// shim launched via fork/exec does not. We work around this by passing the
// launch URLs as command line arguments. The function below receives them and
// passes them to the upstream implementation, as if they were received through
// -application:openURLs:.
void AppendTestLaunchUrls(std::vector<GURL>& launch_urls) {
  const std::string_view prefix(app_mode::kTestLaunchUrlPrefix);
  for (const auto& arg : base::CommandLine::ForCurrentProcess()->GetArgs()) {
    if (!base::StartsWith(arg, prefix)) {
      continue;
    }
    GURL url(arg.substr(prefix.size()));
    if (url.is_valid()) {
      launch_urls.push_back(url);
    }
  }
}

}  // namespace

#include <chrome/app_shim/app_shim_controller.mm>
