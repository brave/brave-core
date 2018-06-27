/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"

using BraveProfilePrefsBrowserTest = InProcessBrowserTest;

// Check download prompt preference is set to true by default.
IN_PROC_BROWSER_TEST_F(BraveProfilePrefsBrowserTest, DownloadPromptDefault) {
  EXPECT_TRUE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kPromptForDownload));
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(kWidevineOptedIn));
}
