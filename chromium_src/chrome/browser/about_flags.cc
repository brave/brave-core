/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/about_flags.h"

#include "base/strings/string_util.h"
#include "brave/common/brave_features.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "brave/components/ipfs/browser/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/speedreader/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

using brave_shields::features::kBraveAdblockCosmeticFiltering;
using ntp_background_images::features::kBraveNTPBrandedWallpaper;
using ntp_background_images::features::kBraveNTPBrandedWallpaperDemo;
using ntp_background_images::features::kBraveNTPSuperReferralWallpaper;


#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/features.h"

#define SPEEDREADER_FEATURE_ENTRIES \
    {"brave-speedreader",                                                  \
     flag_descriptions::kBraveSpeedreaderName,                             \
     flag_descriptions::kBraveSpeedreaderDescription, kOsDesktop,          \
     FEATURE_VALUE_TYPE(speedreader::kSpeedreaderFeature)},
#else
#define SPEEDREADER_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "brave/components/brave_sync/features.h"

#define BRAVE_SYNC_FEATURE_ENTRIES                                         \
    {"brave-sync-v2",                                                      \
     flag_descriptions::kBraveSyncName,                                    \
     flag_descriptions::kBraveSyncDescription, kOsDesktop,                 \
     FEATURE_VALUE_TYPE(brave_sync::features::kBraveSync)},
#else
#define BRAVE_SYNC_FEATURE_ENTRIES
#endif

#if !defined(OS_ANDROID)
  #define BRAVE_FEATURE_ENTRIES_COMMA ,
#else
  #define BRAVE_FEATURE_ENTRIES_COMMA
#endif // !defined(OS_ANDROID)

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/components/ipfs/browser/features.h"

#define BRAVE_IPFS_FEATURE_ENTRIES                                         \
    {"brave-ipfs",                                                         \
     flag_descriptions::kBraveIpfsName,                                    \
     flag_descriptions::kBraveIpfsDescription, kOsDesktop,                 \
     FEATURE_VALUE_TYPE(ipfs::features::kIpfsFeature)},
#else
#define BRAVE_IPFS_FEATURE_ENTRIES
#endif

#define BRAVE_FEATURE_ENTRIES \
    BRAVE_FEATURE_ENTRIES_COMMA                                            \
    {"use-dev-updater-url",                                                \
     flag_descriptions::kUseDevUpdaterUrlName,                             \
     flag_descriptions::kUseDevUpdaterUrlDescription, kOsAll,              \
     FEATURE_VALUE_TYPE(brave_component_updater::kUseDevUpdaterUrl)},      \
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
    SPEEDREADER_FEATURE_ENTRIES                                            \
    BRAVE_SYNC_FEATURE_ENTRIES                                             \
    BRAVE_IPFS_FEATURE_ENTRIES                                             \
    {"brave-super-referral",                                               \
     flag_descriptions::kBraveSuperReferralName,                           \
     flag_descriptions::kBraveSuperReferralDescription,                    \
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsAndroid,           \
     FEATURE_VALUE_TYPE(kBraveNTPSuperReferralWallpaper)},

#define SetFeatureEntryEnabled SetFeatureEntryEnabled_ChromiumImpl
#include "../../../../chrome/browser/about_flags.cc"  // NOLINT
#undef SetFeatureEntryEnabled
#undef BRAVE_FEATURE_ENTRIES

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
