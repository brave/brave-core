/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_features.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "chrome/browser/about_flags.h"

using brave_shields::features::kBraveAdblockCosmeticFiltering;
using brave_sync::features::kBraveSync;
using ntp_background_images::features::kBraveNTPBrandedWallpaper;
using ntp_background_images::features::kBraveNTPBrandedWallpaperDemo;

#define BRAVE_FEATURE_ENTRIES \
    {"use-dev-updater-url",                                                \
     flag_descriptions::kUseDevUpdaterUrlName,                             \
     flag_descriptions::kUseDevUpdaterUrlDescription, kOsAll,              \
     FEATURE_VALUE_TYPE(features::kUseDevUpdaterUrl)},                     \
    {"brave-ntp-branded-wallpaper",                                        \
     flag_descriptions::kBraveNTPBrandedWallpaperName,                     \
     flag_descriptions::kBraveNTPBrandedWallpaperDescription, kOsAll,      \
     FEATURE_VALUE_TYPE(kBraveNTPBrandedWallpaper)},                       \
    {"brave-ntp-branded-wallpaper-demo",                                   \
     flag_descriptions::kBraveNTPBrandedWallpaperDemoName,                 \
     flag_descriptions::kBraveNTPBrandedWallpaperDemoDescription, kOsAll,  \
     FEATURE_VALUE_TYPE(kBraveNTPBrandedWallpaperDemo)},                   \
    {"brave-adblock-cosmetic-filtering",                                   \
     flag_descriptions::kBraveAdblockCosmeticFilteringName,                \
     flag_descriptions::kBraveAdblockCosmeticFilteringDescription, kOsAll, \
     FEATURE_VALUE_TYPE(kBraveAdblockCosmeticFiltering)},                  \
    {"brave-sync",                                                         \
     flag_descriptions::kBraveSyncName,                                    \
     flag_descriptions::kBraveSyncDescription, kOsDesktop,                 \
     FEATURE_VALUE_TYPE(kBraveSync)},

#define SetFeatureEntryEnabled SetFeatureEntryEnabled_ChromiumImpl
#include "../../../../chrome/browser/about_flags.cc"  // NOLINT
#undef SetFeatureEntryEnabled

#include "base/strings/string_util.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace about_flags {

void UpdateBraveMediaRouterPref(const std::string& internal_name,
                                Profile* profile) {
  bool enable = false;
  if (base::EndsWith(internal_name, "@1", base::CompareCase::SENSITIVE)) {
    enable = true;
  }
  if (base::StartsWith(internal_name, "load-media-router-component-extension",
                       base::CompareCase::SENSITIVE)) {
    profile->GetPrefs()->SetBoolean(kBraveEnabledMediaRouter, enable);
  }
}

void SetFeatureEntryEnabled(flags_ui::FlagsStorage* flags_storage,
                            const std::string& internal_name,
                            bool enable) {
  UpdateBraveMediaRouterPref(internal_name,
                             ProfileManager::GetActiveUserProfile());
  SetFeatureEntryEnabled_ChromiumImpl(flags_storage, internal_name, enable);
}

}  // namespace about_flags
