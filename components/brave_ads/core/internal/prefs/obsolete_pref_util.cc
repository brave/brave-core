/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr char kNotificationAdLastNormalizedDisplayCoordinateX[] =
    "brave.brave_ads.ad_notification.last_normalized_display_coordinate_x";
constexpr char kNotificationAdLastNormalizedDisplayCoordinateY[] =
    "brave.brave_ads.ad_notification.last_normalized_display_coordinate_y";

}  // namespace

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 11/2023.
  registry->RegisterDoublePref(kNotificationAdLastNormalizedDisplayCoordinateX,
                               0.0);
  registry->RegisterDoublePref(kNotificationAdLastNormalizedDisplayCoordinateY,
                               0.0);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 11/2023.
  prefs->ClearPref(kNotificationAdLastNormalizedDisplayCoordinateX);
  prefs->ClearPref(kNotificationAdLastNormalizedDisplayCoordinateY);
}

}  // namespace brave_ads
