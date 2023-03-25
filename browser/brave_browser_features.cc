/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_features.h"
#include "build/build_config.h"

namespace features {

// Sanitize url before copying, replaces default ctrl+c hotkey for urls.
BASE_FEATURE(kBraveCopyCleanLinkByDefault,
             "brave-copy-clean-link-by-default",
#if BUILDFLAG(IS_MAC)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

}  // namespace features
