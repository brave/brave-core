/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
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
};

#if BUILDFLAG(IS_WIN)
IN_PROC_BROWSER_TEST_P(BraveTabsSearchButtonTest, HideShowSettingTest) {
#else
IN_PROC_BROWSER_TEST_F(BraveTabsSearchButtonTest, HideShowSettingTest) {
#endif
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(kTabsSearchShow));

  views::View* button = nullptr;
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* tab_search_container =
      browser_view->tab_strip_region_view()->tab_search_container();
  if (!tab_search_container) {
    return;
  }
  button = browser_view->tab_strip_region_view()
               ->tab_search_container()
               ->tab_search_button();
  ASSERT_NE(nullptr, button);
  EXPECT_TRUE(button->GetVisible());

  prefs->SetBoolean(kTabsSearchShow, false);
  EXPECT_FALSE(button->GetVisible());
  prefs->SetBoolean(kTabsSearchShow, true);
  EXPECT_TRUE(button->GetVisible());
}

#if BUILDFLAG(IS_WIN)
INSTANTIATE_TEST_SUITE_P(BraveTabsSearchButtonTest,
                         BraveTabsSearchButtonTest,
                         ::testing::Bool());
#endif
