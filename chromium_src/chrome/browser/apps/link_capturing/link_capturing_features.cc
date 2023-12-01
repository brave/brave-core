/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/apps/link_capturing/link_capturing_features.cc"

#include "base/feature_override.h"

namespace apps::features {

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kDesktopPWAsLinkCapturing, base::FEATURE_DISABLED_BY_DEFAULT},
}});
#endif

}  // namespace apps::features
