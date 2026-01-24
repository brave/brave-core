/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

// This override allows Brave to define its own IPH features, and to modify the
// IPH features defined by Chromium.

#include <components/feature_engagement/public/feature_constants.cc>

namespace feature_engagement {

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
BASE_FEATURE(kIPHBraveShieldsInPageInfoFeature,
             "IPH_BraveShieldsInPageInfo",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX)
    {kIPHGMCCastStartStopFeature, base::FEATURE_DISABLED_BY_DEFAULT},
    {kIPHPasswordsManagementBubbleAfterSaveFeature,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kIPHPdfInkSignaturesFeature, base::FEATURE_DISABLED_BY_DEFAULT},
    {kIPHTabGroupsSaveV2IntroFeature, base::FEATURE_DISABLED_BY_DEFAULT},
    {kIPHTabSearchToolbarButtonFeature, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
}});

}  // namespace feature_engagement
