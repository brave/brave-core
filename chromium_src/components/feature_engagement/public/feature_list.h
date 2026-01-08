/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_FEATURE_LIST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_FEATURE_LIST_H_

#include "build/build_config.h"

// This override allows Brave to display Brave-specific in-product help features
// on the brave://flags page.

// BRAVE_FEATURE_ENGAGEMENT_VARIATION_PARAMS is patched in after Chromium's
// variation params are defined, and allows us to define Brave-specific IPH
// variation params. A patch is necessary because the DEFINE_VARIATION_PARAM
// macro is undef'ed at the end of the header file.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#define BRAVE_FEATURE_ENGAGEMENT_VARIATION_PARAMS           \
  DEFINE_VARIATION_PARAM(kIPHBraveShieldsInPageInfoFeature, \
                         "IPH_BraveShieldsInPageInfo");
#else
#define BRAVE_FEATURE_ENGAGEMENT_VARIATION_PARAMS
#endif

// BRAVE_FEATURE_ENGAGEMENT_VARIATION_ENTRIES is patched in at the start of the
// kIPHDemoModeChoiceVariations array, and allows us to add Brave-specific IPH
// variation entries to the array.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#define BRAVE_FEATURE_ENGAGEMENT_VARIATION_ENTRIES \
  VARIATION_ENTRY(kIPHBraveShieldsInPageInfoFeature),
#else
#define BRAVE_FEATURE_ENGAGEMENT_VARIATION_ENTRIES
#endif

#include <components/feature_engagement/public/feature_list.h>  // IWYU pragma: export

#undef BRAVE_FEATURE_ENGAGEMENT_VARIATION_PARAMS
#undef BRAVE_FEATURE_ENGAGEMENT_VARIATION_ENTRIES

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_FEATURE_LIST_H_
