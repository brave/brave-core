/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/feature_engagement/public/feature_list.h"

// This override adds Brave-specific IPH features to the kAllFeatures array,
// which is used by the feature engagement tracker and GetAllFeatures().

// Replaces the first entry in the kAllFeatures array with that entry, plus any
// additional entries for Brave-specific IPH features.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#define kIPHDummyFeature kIPHDummyFeature, &kIPHBraveShieldsInPageInfoFeature
#endif

#include <components/feature_engagement/public/feature_list.cc>

#undef kIPHDummyFeature
