/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_AVATAR_ICON_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_AVATAR_ICON_UTIL_H_

#include "build/build_config.h"

#include "src/chrome/browser/profiles/profile_avatar_icon_util.h"  // IWYU pragma: export

namespace profiles {

#if !BUILDFLAG(IS_CHROMEOS) && !BUILDFLAG(IS_ANDROID)
inline constexpr size_t kBraveDefaultAvatarIconsCount = 34;
#else
inline constexpr size_t kBraveDefaultAvatarIconsCount = 0;
#endif

// Provide direct access to custom implementation
base::Value::Dict GetDefaultProfileAvatarIconAndLabel_Brave(
    SkColor fill_color,
    SkColor stroke_color,
    bool selected);
}  // namespace profiles

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_AVATAR_ICON_UTIL_H_
