/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"

using BraveSharingHubTest = InProcessBrowserTest;

namespace sharing_hub {

IN_PROC_BROWSER_TEST_F(BraveSharingHubTest, SharingHubIconVisibility) {
  views::View* sharing_hub_icon =
      BrowserView::GetBrowserViewForBrowser(browser())
          ->toolbar_button_provider()
          ->GetPageActionIconView(PageActionIconType::kSharingHub);

  // No icon.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab/")));
  EXPECT_FALSE(sharing_hub_icon->GetVisible());
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings/")));
  EXPECT_FALSE(sharing_hub_icon->GetVisible());

  // Visible icon.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com/")));
  EXPECT_TRUE(sharing_hub_icon->GetVisible());
}

}  // namespace sharing_hub
