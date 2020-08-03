// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_ANDROID_PREFERENCES_BRAVE_PREFS_H_
#define BRAVE_BROWSER_ANDROID_PREFERENCES_BRAVE_PREFS_H_

#include <cstddef>

#include "brave/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "build/build_config.h"

// A preference exposed to Java.
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.preferences
enum BravePref {
  NTP_SHOW_SPONSORED_IMAGES_BACKGROUND_IMAGE = 10000,
  NTP_SHOW_SUPER_REFERRAL_THEMES_OPTION,
  NTP_SHOW_BACKGROUND_IMAGE,
  TOP_SITE_SUGGESTIONS_ENABLED,
  BRAVE_SUGGESTED_SITE_SUGGESTIONS_ENABLED,
  BRAVE_REWARDS_ENABLED,
  // BRAVE_PREF_NUM_PREFS must be the last entry.
  BRAVE_PREF_NUM_PREFS
};

// The indices must match value of Pref.
const char* const kBravePrefsExposedToJava[] = {
    ntp_background_images::prefs::kNewTabPageShowSponsoredImagesBackgroundImage,
    ntp_background_images::prefs::kNewTabPageSuperReferralThemesOption,
    ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
    kTopSiteSuggestionsEnabled,
    kBraveSuggestedSiteSuggestionsEnabled,
    brave_rewards::prefs::kBraveRewardsEnabled,
};

static const int kBravePrefOffset = 10000;

#endif  // BRAVE_BROWSER_ANDROID_PREFERENCES_BRAVE_PREFS_H_
