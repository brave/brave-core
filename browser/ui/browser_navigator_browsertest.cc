// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/blocked_content/popup_blocker_tab_helper.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

class BrowserNavigatorPopupAsTabBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  BrowserNavigatorPopupAsTabBrowserTest() {
    GetParam() ? feature_list_.InitAndEnableFeature(
                     features::kForcePopupToBeOpenedAsTab)
               : feature_list_.InitAndDisableFeature(
                     features::kForcePopupToBeOpenedAsTab);
  }
  ~BrowserNavigatorPopupAsTabBrowserTest() override = default;

  bool ShouldOpenPopupAsTab() const {
    return base::FeatureList::IsEnabled(features::kForcePopupToBeOpenedAsTab);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_P(BrowserNavigatorPopupAsTabBrowserTest, OpenPopupAsTab) {
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  auto* web_contents = browser()->GetTabStripModel()->GetWebContentsAt(0);
  ASSERT_TRUE(content::ExecJs(
      web_contents->GetPrimaryMainFrame(),
      "window.open('about:blank', '_blank', 'height=200,width=150');"));
  auto* popup_blocker_tab_helper =
      blocked_content::PopupBlockerTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(popup_blocker_tab_helper);

  if (ShouldOpenPopupAsTab()) {
    EXPECT_TRUE(base::test::RunUntil([&]() {
      if (popup_blocker_tab_helper->GetBlockedPopupsCount() != 0) {
        popup_blocker_tab_helper->ShowAllBlockedPopups();
      }
      return browser()->tab_strip_model()->count() == 2;
    }));
    EXPECT_EQ(1u, BrowserList::GetInstance()->size());
  } else {
    EXPECT_TRUE(base::test::RunUntil([&]() {
      if (popup_blocker_tab_helper->GetBlockedPopupsCount() != 0) {
        popup_blocker_tab_helper->ShowAllBlockedPopups();
      }
      return BrowserList::GetInstance()->size() == 2;
    }));
    for (auto& b : *BrowserList::GetInstance()) {
      if (b == browser()) {
        continue;
      }

      EXPECT_TRUE(b->is_type_popup());
    }
    EXPECT_EQ(1, browser()->tab_strip_model()->count());
  }
}

INSTANTIATE_TEST_SUITE_P(All,
                         BrowserNavigatorPopupAsTabBrowserTest,
                         ::testing::Bool());
