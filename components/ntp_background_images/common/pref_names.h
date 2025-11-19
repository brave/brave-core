// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_PREF_NAMES_H_

namespace ntp_background_images::prefs {

inline constexpr char kNewTabPageSponsoredImagesSurveyPanelist[] =
    "brave.new_tab_page.sponsored_images.survey_panelist";
inline constexpr char kBrandedWallpaperNotificationDismissed[] =
    "brave.branded_wallpaper_notification_dismissed";
inline constexpr char kNewTabPageShowSponsoredImagesBackgroundImage[] =
    "brave.new_tab_page.show_branded_background_image";
inline constexpr char kNewTabPageShowBackgroundImage[] =
    "brave.new_tab_page.show_background_image";
inline constexpr char kNewTabTakeoverInfobarRemainingDisplayCount[] =
    "brave.new_tab_page.new_tab_takeover_infobar_remaining_display_count";

}  // namespace ntp_background_images::prefs

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_COMMON_PREF_NAMES_H_
