// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/common/pref_names.h"

namespace ntp_background_images {
namespace prefs {

const char kBrandedWallpaperNotificationDismissed[] =
    "brave.branded_wallpaper_notification_dismissed";
const char kNewTabPageShowSponsoredImagesBackgroundImage[] =
    "brave.new_tab_page.show_branded_background_image";
const char kNewTabPageShowSuperReferrerBackgroundImage[] =
    "brave.new_tab_page.show_super_referrer_background_image";
const char kNewTabPageShowBackgroundImage[] =
    "brave.new_tab_page.show_background_image";

extern const char kNewTabPageCachedReferralPromoCode[] =
    "brave.new_tab_page.cached_referral_promo_code";

extern const char kNewTabPageCachedSuperReferrerComponentInfo[] =
    "brave.new_tab_page.cached_super_referral_component_info";
}  // namespace prefs
}  // namespace ntp_background_images
