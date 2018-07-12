/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_switches.h"
#include "chrome/browser/google/chrome_google_url_tracker_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"

using BraveGoogleURLTrackerClientTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveGoogleURLTrackerClientTest, DisablesGoogleURLTrackerClient) {
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableChromeGoogleURLTrackingClient));

  ChromeGoogleURLTrackerClient client(browser()->profile());
  ASSERT_FALSE(client.IsBackgroundNetworkingEnabled());
}
