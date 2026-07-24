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

// Appends URLs on the command line that are tagged with `kTestLaunchUrlPrefix`
// to `launch_urls`.
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
