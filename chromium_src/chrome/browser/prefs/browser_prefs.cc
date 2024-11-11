/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/brave_rewards/rewards_prefs_util.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/misc_metrics/uptime_monitor.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/translate/brave_translate_prefs_migration.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/brave_adaptive_captcha/prefs_util.h"
#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/common/p3a_pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_shields/content/browser/brave_shields_p3a.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ipfs/ipfs_prefs.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/p3a/star_randomness_meta.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_utils.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#define MigrateObsoleteLocalStatePrefs \
  MigrateObsoleteLocalStatePrefs_ChromiumImpl
#include "src/chrome/browser/prefs/browser_prefs.cc"
#undef MigrateObsoleteProfilePrefs
#undef MigrateObsoleteLocalStatePrefs

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_utils.h"
#endif

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/browser/pref_names.h"
#endif

// This method should be periodically pruned of year+ old migrations.
void MigrateObsoleteProfilePrefs(PrefService* profile_prefs,
                                 const base::FilePath& profile_path) {
  DCHECK(profile_prefs);
  // BEGIN_MIGRATE_OBSOLETE_PROFILE_PREFS
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // Added 02/2020.
  // Must be called before ChromiumImpl because it's migrating a Chromium pref
  // to Brave pref.
  gcm::MigrateGCMPrefs(profile_prefs);
#endif

  MigrateObsoleteProfilePrefs_ChromiumImpl(profile_prefs, profile_path);

  brave_sync::MigrateBraveSyncPrefs(profile_prefs);

#if !BUILDFLAG(IS_ANDROID)
  // Added 10/2022
  profile_prefs->ClearPref(kDefaultBrowserLaunchingCount);
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Added 11/2022
  profile_prefs->ClearPref(kDontAskEnableWebDiscovery);
  profile_prefs->ClearPref(kBraveSearchVisitCount);
#endif

  brave_wallet::MigrateObsoleteProfilePrefs(profile_prefs);

  // Added 05/2021
  profile_prefs->ClearPref(kBraveNewsIntroDismissed);
  // Added 07/2021
  profile_prefs->ClearPref(prefs::kNetworkPredictionOptions);

  // Added 01/2022
  brave_rewards::MigrateObsoleteProfilePrefs(profile_prefs);

  // Added 05/2022
  translate::ClearMigrationBraveProfilePrefs(profile_prefs);

  // Added 06/2022
#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  NTPBackgroundPrefs(profile_prefs).MigrateOldPref();
#endif

  // Added 24/11/2022: https://github.com/brave/brave-core/pull/16027
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kFTXAccessToken);
  profile_prefs->ClearPref(kFTXOauthHost);
  profile_prefs->ClearPref(kFTXNewTabPageShowFTX);
  profile_prefs->ClearPref(kCryptoDotComNewTabPageShowCryptoDotCom);
  profile_prefs->ClearPref(kCryptoDotComHasBoughtCrypto);
  profile_prefs->ClearPref(kCryptoDotComHasInteracted);
  profile_prefs->ClearPref(kGeminiAccessToken);
  profile_prefs->ClearPref(kGeminiRefreshToken);
  profile_prefs->ClearPref(kNewTabPageShowGemini);
#endif

  // Added 24/11/2022: https://github.com/brave/brave-core/pull/16027
#if !BUILDFLAG(IS_IOS)
  profile_prefs->ClearPref(kBinanceAccessToken);
  profile_prefs->ClearPref(kBinanceRefreshToken);
  profile_prefs->ClearPref(kNewTabPageShowBinance);
  profile_prefs->ClearPref(kBraveSuggestedSiteSuggestionsEnabled);
#endif

  // Added 03/2024
#if BUILDFLAG(ENABLE_TOR)
  profile_prefs->ClearPref(tor::prefs::kAutoOnionRedirect);
#endif

#if defined(TOOLKIT_VIEWS)
  // Added May 2023
  if (profile_prefs->GetBoolean(sidebar::kSidebarAlignmentChangedTemporarily)) {
    // If temporarily changed, it means sidebar is set to right.
    // Just clear alignment prefs as default alignment is changed to right.
    profile_prefs->ClearPref(prefs::kSidePanelHorizontalAlignment);
  }

  profile_prefs->ClearPref(sidebar::kSidebarAlignmentChangedTemporarily);
#endif

  brave_news::p3a::prefs::MigrateObsoleteProfileNewsMetricsPrefs(profile_prefs);

  // Added 2023-09
  ntp_background_images::ViewCounterService::MigrateObsoleteProfilePrefs(
      profile_prefs);

  // Added 2023-11
  brave_ads::MigrateObsoleteProfilePrefs(profile_prefs);

  brave_shields::MigrateObsoleteProfilePrefs(profile_prefs);

#if !BUILDFLAG(IS_ANDROID)
  // Added 2024-01
  brave_tabs::MigrateBraveProfilePrefs(profile_prefs);
#endif  // !BUILDFLAG(IS_ANDROID)

  // Added 2024-04
  ai_chat::ModelService::MigrateProfilePrefs(profile_prefs);

  // Added 2024-05
  ipfs::ClearDeprecatedIpfsPrefs(profile_prefs);

  // Added 2024-07
  profile_prefs->ClearPref(kHangoutsEnabled);

  // Added 2024-10
  brave_adaptive_captcha::MigrateObsoleteProfilePrefs(profile_prefs);

  // END_MIGRATE_OBSOLETE_PROFILE_PREFS
}

// This method should be periodically pruned of year+ old migrations.
void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // BEGIN_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
  MigrateObsoleteLocalStatePrefs_ChromiumImpl(local_state);

#if BUILDFLAG(ENABLE_TOR)
  // Added 4/2021.
  tor::MigrateLastUsedProfileFromLocalStatePrefs(local_state);
#endif

  decentralized_dns::MigrateObsoleteLocalStatePrefs(local_state);

#if !BUILDFLAG(IS_ANDROID)
  // Added 10/2022
  local_state->ClearPref(kDefaultBrowserPromptEnabled);
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // Added 09/2024
  brave_vpn::MigrateLocalStatePrefs(local_state);
#endif

  misc_metrics::UptimeMonitor::MigrateObsoletePrefs(local_state);
  brave_search_conversion::p3a::MigrateObsoleteLocalStatePrefs(local_state);
  brave_stats::MigrateObsoleteLocalStatePrefs(local_state);
  p3a::StarRandomnessMeta::MigrateObsoleteLocalStatePrefs(local_state);

  // END_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
}

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#undef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)
