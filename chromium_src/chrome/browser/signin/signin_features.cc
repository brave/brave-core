/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/signin/signin_features.h"

#include "src/chrome/browser/signin/signin_features.cc"

#include "base/feature_override.h"

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if !BUILDFLAG(IS_ANDROID)
    {kForYouFre, base::FEATURE_DISABLED_BY_DEFAULT},
#endif  // !BUILDFLAG(IS_ANDROID)
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
    {kPreconnectAccountCapabilitiesBeforeSignIn,
     base::FEATURE_DISABLED_BY_DEFAULT},
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)
#if BUILDFLAG(ENABLE_MIRROR)
    {kVerifyRequestInitiatorForMirrorHeaders,
     base::FEATURE_DISABLED_BY_DEFAULT},
#endif  // BUILDFLAG(ENABLE_MIRROR)
}});
