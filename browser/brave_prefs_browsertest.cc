/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/metrics/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefetch/pref_names.h"
#include "chrome/browser/preloading/preloading_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/embedder_support/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/sync/base/pref_names.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/webui/bookmarks/bookmark_prefs.h"
#include "chrome/browser/ui/webui/side_panel/bookmarks/bookmarks.mojom.h"
#endif

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#endif

using BraveProfilePrefsBrowserTest = PlatformBrowserTest;
using BraveLocalStatePrefsBrowserTest = PlatformBrowserTest;

// Check download prompt preference is set to true by default.
IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, DownloadPromptDefault) {
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kPromptForDownload));
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, MiscBravePrefs) {
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kNoScriptControlType));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kShieldsAdvancedViewEnabled));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kShieldsStatsBadgeVisible));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kAdControlType));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kGoogleLoginControlType));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      brave_shields::prefs::kFBEmbedControlType));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      brave_shields::prefs::kTwitterEmbedControlType));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      brave_shields::prefs::kLinkedInEmbedControlType));
  EXPECT_EQ(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
                brave_shields::prefs::kReduceLanguageEnabled),
            base::FeatureList::IsEnabled(
                brave_shields::features::kBraveReduceLanguage));
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kWebTorrentEnabled));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kBraveWaybackMachineEnabled));
#endif
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kShowLocationBarButton));
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kERCOptedIntoCryptoWallets));
#endif
  EXPECT_EQ(brave_wallet::GetDefaultEthereumWallet(
                chrome_test_utils::GetProfile(this)->GetPrefs()),
            brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  EXPECT_EQ(brave_wallet::GetDefaultSolanaWallet(
                chrome_test_utils::GetProfile(this)->GetPrefs()),
            brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kShowWalletIconOnToolbar));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kMRUCyclingEnabled));
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kBraveGCMChannelStatus));
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      brave_vpn::prefs::kBraveVPNShowButton));
#endif

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->HasPrefPath(
      NTPBackgroundPrefs::kDeprecatedPrefName));
#endif

#if !BUILDFLAG(IS_ANDROID)
  EXPECT_EQ(static_cast<int>(side_panel::mojom::ViewType::kCompact),
            chrome_test_utils::GetProfile(this)->GetPrefs()->GetInteger(
                bookmarks_webui::prefs::kBookmarksViewType));
#endif
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest,
                       DisableGoogleServicesByDefault) {
#if defined(TOOLKIT_VIEWS)
  constexpr char kSideSearchEnabled[] = "side_search.enabled";
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kSideSearchEnabled));
#endif
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      embedder_support::kAlternateErrorPagesEnabled));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kSafeBrowsingExtendedReportingOptInAllowed));
#if !BUILDFLAG(IS_ANDROID)
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kSearchSuggestEnabled));
#endif
  EXPECT_EQ(chrome_test_utils::GetProfile(this)->GetPrefs()->GetInteger(
                prefetch::prefs::kNetworkPredictionOptions),
            static_cast<int>(prefetch::NetworkPredictionOptions::kDisabled));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kSigninAllowedOnNextStartup));
  // Verify cloud print is disabled.
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kCloudPrintProxyEnabled));
#if !BUILDFLAG(IS_ANDROID)
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      ntp_prefs::kNtpUseMostVisitedTiles));
#endif
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      policy::policy_prefs::kHideWebStoreIcon));
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, MediaRouterPrefTest) {
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      ::prefs::kEnableMediaRouter));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kEnableMediaRouterOnRestart));
}

IN_PROC_BROWSER_TEST_F(BraveLocalStatePrefsBrowserTest, DefaultLocalStateTest) {
#if BUILDFLAG(ENABLE_CRASH_DIALOG)
  EXPECT_FALSE(
      g_browser_process->local_state()->GetBoolean(kDontAskForCrashReporting));
#endif
}
