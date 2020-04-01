// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_PREF_NAMES_H_

namespace ntp_background_images {
namespace prefs {

// There are two categories in Branded wallpaper.
// The one is sponsored images wallpaper and the other is super referral
// wallpaper.
extern const char kNewTabPageShowSponsoredImagesBackgroundImage[];
extern const char kNewTabPageSuperReferralThemesOption[];
extern const char kBrandedWallpaperNotificationDismissed[];
extern const char kNewTabPageShowBackgroundImage[];

// Local prefs
extern const char kNewTabPageCachedSuperReferralComponentInfo[];
extern const char kNewTabPageCachedSuperReferralFaviconList[];

}  // namespace prefs
}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_PREF_NAMES_H_
