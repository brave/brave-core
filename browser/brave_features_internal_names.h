/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_FEATURES_INTERNAL_NAMES_H_
#define BRAVE_BROWSER_BRAVE_FEATURES_INTERNAL_NAMES_H_

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/build_config.h"

inline constexpr char kPlaylistFeatureInternalName[] = "playlist";
inline constexpr char kPlaylistFakeUAFeatureInternalName[] = "playlist-fake-ua";

#if !BUILDFLAG(IS_ANDROID)
inline constexpr char kSplitViewFeatureInternalName[] = "brave-split-view";
#endif

#endif  // BRAVE_BROWSER_BRAVE_FEATURES_INTERNAL_NAMES_H_
