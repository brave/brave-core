/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/embedder_support/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/sync/base/pref_names.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

using BraveProfilePrefsBrowserTest = PlatformBrowserTest;
using BraveLocalStatePrefsBrowserTest = PlatformBrowserTest;

// Check download prompt preference is set to true by default.
IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, DownloadPromptDefault) {
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kPromptForDownload));
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, MiscBravePrefs) {
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kHTTPSEVerywhereControlType));
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
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kWebTorrentEnabled));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kBraveWaybackMachineEnabled));
#endif
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kHangoutsEnabled));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kHideButton));
#if BUILDFLAG(IPFS_ENABLED)
  EXPECT_EQ(chrome_test_utils::GetProfile(this)->GetPrefs()->GetInteger(
                kIPFSResolveMethod),
            static_cast<int>((ipfs::IPFSResolveMethodTypes::IPFS_ASK)));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)
                  ->GetPrefs()
                  ->GetFilePath(kIPFSBinaryPath)
                  .empty());
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kIPFSAutoRedirectGateway));
#endif
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kIPFSCompanionEnabled));
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kERCLoadCryptoWalletsOnStartup));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kERCOptedIntoCryptoWallets));
#endif
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  EXPECT_EQ(chrome_test_utils::GetProfile(this)->GetPrefs()->GetInteger(
                kBraveWalletWeb3Provider),
            static_cast<int>(brave_wallet::IsNativeWalletEnabled()
                                 ? brave_wallet::Web3ProviderTypes::BRAVE_WALLET
                                 : brave_wallet::Web3ProviderTypes::ASK));
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kShowWalletIconOnToolbar));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kBraveWalletBackupComplete));
#endif
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kMRUCyclingEnabled));
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      kBraveGCMChannelStatus));
#endif
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest,
                       DisableGoogleServicesByDefault) {
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      embedder_support::kAlternateErrorPagesEnabled));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kSafeBrowsingExtendedReportingOptInAllowed));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kSearchSuggestEnabled));
  EXPECT_EQ(chrome_test_utils::GetProfile(this)->GetPrefs()->GetInteger(
                prefs::kNetworkPredictionOptions),
            chrome_browser_net::NETWORK_PREDICTION_NEVER);
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kSigninAllowedOnNextStartup));
  // Verify cloud print is disabled.
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kCloudPrintProxyEnabled));
  EXPECT_FALSE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kCloudPrintSubmitEnabled));
#if !defined(OS_ANDROID)
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kNtpUseMostVisitedTiles));
#endif
  EXPECT_TRUE(chrome_test_utils::GetProfile(this)->GetPrefs()->GetBoolean(
      prefs::kHideWebStoreIcon));
}

#if !defined(OS_ANDROID)
IN_PROC_BROWSER_TEST_F(BraveLocalStatePrefsBrowserTest, DefaultLocalStateTest) {
  EXPECT_TRUE(g_browser_process->local_state()->GetBoolean(
      kDefaultBrowserPromptEnabled));
}
#endif
