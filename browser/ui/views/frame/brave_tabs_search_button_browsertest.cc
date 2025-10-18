/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/views/tab_search_bubble_host.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
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

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  views::View* button = nullptr;
  if (features::HasTabSearchToolbarButton()) {
    button = browser_view->toolbar()->tab_search_button();
  } else {
    button = BrowserElementsViews::From(browser())->GetViewAs<TabSearchButton>(
        kTabSearchButtonElementId);
  }
  ASSERT_TRUE(button);
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
