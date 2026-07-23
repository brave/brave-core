/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "brave/components/constants/mac_app_mode_common.h"

// A shim spawned directly for a browser test (rather than opened through
// LaunchServices) receives its launch URLs as positional command-line
// arguments, each tagged with kTestLaunchUrlPrefix, instead of through
// -application:openURLs:. Recover the tagged arguments here. The tag is only
// ever emitted by web_app::LaunchShimForTesting, so this is inert outside of
// that test path.
#define BRAVE_SEND_BOOTSTRAP_ON_SHIM_CONNECTED                     \
  {                                                                \
    const std::string_view prefix(app_mode::kTestLaunchUrlPrefix); \
    for (const auto& arg :                                         \
         base::CommandLine::ForCurrentProcess()->GetArgs()) {      \
      if (!base::StartsWith(arg, prefix)) {                        \
        continue;                                                  \
      }                                                            \
      GURL url(arg.substr(prefix.size()));                         \
      if (url.is_valid()) {                                        \
        launch_urls_.push_back(url);                               \
      }                                                            \
    }                                                              \
  }

#include <chrome/app_shim/app_shim_controller.mm>

#undef BRAVE_SEND_BOOTSTRAP_ON_SHIM_CONNECTED
