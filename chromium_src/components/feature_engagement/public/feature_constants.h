/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_FEATURE_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_FEATURE_CONSTANTS_H_

// This override allows Brave to declare its own IPH features. Features declared
// here must be defined in "feature_constants.cc".

#include <components/feature_engagement/public/feature_constants.h>  // IWYU pragma: export

namespace feature_engagement {

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// IPH for notifying users that Brave Shields settings have moved to Page Info.
FEATURE_CONSTANTS_DECLARE_FEATURE(kIPHBraveShieldsInPageInfoFeature);
#endif

}  // namespace feature_engagement

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_FEATURE_CONSTANTS_H_
