/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sharing_hub/sharing_hub_model.h"
#include "chrome/browser/sharing_hub/sharing_hub_service.h"
#include "chrome/browser/sharing_hub/sharing_hub_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

using BraveSharingHubTest = InProcessBrowserTest;

namespace sharing_hub {

IN_PROC_BROWSER_TEST_F(BraveSharingHubTest,
                       SharingHubThirdPartyActionsEmptyTest) {
  auto* profile = browser()->profile();

  EXPECT_TRUE(
      profile->GetPrefs()->GetBoolean(prefs::kDesktopSharingHubEnabled));

  auto* sharing_hub_service = SharingHubServiceFactory::GetForProfile(profile);
  auto* model = sharing_hub_service->GetSharingHubModel();
  std::vector<SharingHubAction> list;
  model->GetThirdPartyActionList(
      browser()->tab_strip_model()->GetActiveWebContents(), &list);
  EXPECT_TRUE(list.empty());
}

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
