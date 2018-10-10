/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/sync/base/pref_names.h"

using BraveProfilePrefsBrowserTest = InProcessBrowserTest;

// Check download prompt preference is set to true by default.
IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, DownloadPromptDefault) {
  EXPECT_TRUE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kPromptForDownload));
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, MiscBravePrefs) {
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(kWidevineOptedIn));
  EXPECT_TRUE(
      browser()->profile()->GetPrefs()->GetBoolean(kHTTPSEVerywhereControlType));
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(kNoScriptControlType));
}

IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, DisableGoogleServicesByDefault) {
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kAlternateErrorPagesEnabled));
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(spellcheck::prefs::kSpellCheckUseSpellingService));
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kSafeBrowsingExtendedReportingOptInAllowed));
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kSearchSuggestEnabled));
  EXPECT_TRUE(
      browser()->profile()->GetPrefs()->GetBoolean(syncer::prefs::kSyncManaged));
  EXPECT_EQ(
      browser()->profile()->GetPrefs()->GetInteger(prefs::kNetworkPredictionOptions),
      chrome_browser_net::NETWORK_PREDICTION_NEVER);

}
