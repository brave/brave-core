// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

class BraveSettingsUIBrowserTest : public InProcessBrowserTest {
 public:
  BraveSettingsUIBrowserTest() = default;
  ~BraveSettingsUIBrowserTest() override = default;
};

// Test that chrome://settings loads without crashing in guest profiles.
// This verifies that all Mojo interface bindings properly handle null services
// for guest profiles (e.g., BraveOriginService).
IN_PROC_BROWSER_TEST_F(BraveSettingsUIBrowserTest, GuestProfileLoadsSettings) {
  // Switch to guest profile and wait for browser to open
  profiles::SwitchToGuestProfile();
  Browser* guest_browser = ui_test_utils::WaitForBrowserToOpen();
  ASSERT_NE(guest_browser, nullptr);

  // Navigate to settings - this should trigger all BindInterface calls
  // including the one for BraveOriginSettingsHandler which needs to handle
  // a null service for guest profiles
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(guest_browser, GURL("chrome://settings/")));

  // Verify navigation succeeded and page loaded
  auto* web_contents = guest_browser->tab_strip_model()->GetActiveWebContents();
  ASSERT_NE(web_contents, nullptr);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(web_contents->GetURL(), GURL("chrome://settings/"));
}
