/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

using BraveBrowserViewTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest, SearchTabsBtnTest) {
  auto* prefs = browser()->profile()->GetPrefs();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* button = browser_view->tab_strip_region_view()->tab_search_button();

  EXPECT_TRUE(prefs->GetBoolean(kTabsSearchShow));
  EXPECT_TRUE(button->GetVisible());

  prefs->SetBoolean(kTabsSearchShow, false);
  EXPECT_FALSE(button->GetVisible());
  prefs->SetBoolean(kTabsSearchShow, true);
  EXPECT_TRUE(button->GetVisible());
}
