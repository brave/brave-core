/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/constants/pref_names.h"

#define BRAVE_MANAGED_CONTENT_SETTINGS                                         \
  {kManagedBraveShieldsDisabledForUrls, ContentSettingsType::BRAVE_SHIELDS,    \
   CONTENT_SETTING_BLOCK},                                                     \
      {kManagedBraveShieldsEnabledForUrls, ContentSettingsType::BRAVE_SHIELDS, \
       CONTENT_SETTING_ALLOW},                                                 \
      {kManagedBraveCosmeticFilteringDisabledForUrls,                          \
       ContentSettingsType::BRAVE_COSMETIC_FILTERING,                          \
       CONTENT_SETTING_DEFAULT},

#define BRAVE_MANAGED_PREFS                                                \
  kManagedBraveShieldsDisabledForUrls, kManagedBraveShieldsEnabledForUrls, \
      kManagedBraveCosmeticFilteringDisabledForUrls,

#define BRAVE_MANAGED_DEFAULT                                                \
  {ContentSettingsType::BRAVE_FINGERPRINTING_V2,                             \
   kManagedDefaultBraveFingerprintingV2},                                    \
      {ContentSettingsType::BRAVE_COOKIES, kManagedDefaultBraveCookies},     \
      {ContentSettingsType::BRAVE_HTTPS_UPGRADE,                             \
       kManagedDefaultBraveHttpsUpgrade},                                    \
      {ContentSettingsType::BRAVE_REFERRERS, kManagedDefaultBraveReferrers}, \
      {ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,                       \
       kManagedDefaultBraveRemember1PStorage},                               \
      {ContentSettingsType::BRAVE_TRACKERS, kManagedDefaultBraveTrackers},   \
      {ContentSettingsType::BRAVE_COSMETIC_FILTERING,                        \
       kManagedDefaultBraveCosmeticFiltering},                               \
      {ContentSettingsType::BRAVE_ADS, kManagedDefaultBraveAds},

#define BRAVE_MANAGED_DEFAULT_PREFS                                        \
  kManagedDefaultBraveFingerprintingV2, kManagedDefaultBraveCookies,       \
      kManagedDefaultBraveHttpsUpgrade, kManagedDefaultBraveReferrers,     \
      kManagedDefaultBraveRemember1PStorage, kManagedDefaultBraveTrackers, \
      kManagedDefaultBraveCosmeticFiltering, kManagedDefaultBraveAds,

#include <components/content_settings/core/browser/content_settings_policy_provider.cc>
#undef BRAVE_MANAGED_DEFAULT_PREFS
#undef BRAVE_MANAGED_DEFAULT
#undef BRAVE_MANAGED_PREFS
#undef BRAVE_MANAGED_CONTENT_SETTINGS
