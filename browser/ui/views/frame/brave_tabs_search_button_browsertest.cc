/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tab_search_bubble_host.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

class BraveTabsSearchButtonTest : public InProcessBrowserTest,
                                  public ::testing::WithParamInterface<bool> {
 public:
  BraveTabsSearchButtonTest() = default;
  ~BraveTabsSearchButtonTest() override = default;

#if defined(OS_WIN)
  bool IsWin10TabSearchCaptionButtonEnabled() { return GetParam(); }

  void SetUp() override {
    if (IsWin10TabSearchCaptionButtonEnabled()) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kWin10TabSearchCaptionButton);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kWin10TabSearchCaptionButton);
    }
    InProcessBrowserTest::SetUp();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
#endif
};

#if defined(OS_WIN)
IN_PROC_BROWSER_TEST_P(BraveTabsSearchButtonTest, HideShowSettingTest) {
#else
IN_PROC_BROWSER_TEST_F(BraveTabsSearchButtonTest, HideShowSettingTest) {
#endif
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(kTabsSearchShow));

  views::View* button = nullptr;
  if (WindowFrameUtil::IsWin10TabSearchCaptionButtonEnabled(browser())) {
    auto* frame_view = BrowserView::GetBrowserViewForBrowser(browser())
                           ->frame()
                           ->GetFrameView();
    auto* tab_search_bubble_host = frame_view->GetTabSearchBubbleHost();
    ASSERT_NE(nullptr, tab_search_bubble_host);
    button = tab_search_bubble_host->button();
  } else {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    button = browser_view->tab_strip_region_view()->tab_search_button();
  }
  ASSERT_NE(nullptr, button);
  EXPECT_TRUE(button->GetVisible());

  prefs->SetBoolean(kTabsSearchShow, false);
  EXPECT_FALSE(button->GetVisible());
  prefs->SetBoolean(kTabsSearchShow, true);
  EXPECT_TRUE(button->GetVisible());
}

#if defined(OS_WIN)
INSTANTIATE_TEST_SUITE_P(BraveTabsSearchButtonTest,
                         BraveTabsSearchButtonTest,
                         ::testing::Bool());
#endif
