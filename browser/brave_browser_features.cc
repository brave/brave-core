/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_features.h"
#include "build/build_config.h"

namespace features {

// Cleanup Session Cookies on browser restart if Session Restore is enabled.
BASE_FEATURE(kBraveCleanupSessionCookiesOnSessionRestore,
             "BraveCleanupSessionCookiesOnSessionRestore",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Sanitize url before copying, replaces default ctrl+c hotkey for urls.
BASE_FEATURE(kBraveCopyCleanLinkByDefault,
             "brave-copy-clean-link-by-default",
#if BUILDFLAG(IS_MAC)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

// Disable download warnings for dangerous files when Safe Browsing is
// disabled.
BASE_FEATURE(kBraveOverrideDownloadDangerLevel,
             "brave-override-download-danger-level",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Adds rounded corners and a drop shadow to the main web content area and side
// bar panels.
BASE_FEATURE(kBraveWebViewRoundedCorners,
             "brave-web-view-rounded-corners",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
