/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget_utils.h"
#include "url/gurl.h"

namespace {

BraveTabStrip* GetBraveTabStrip(BrowserWindowInterface* browser) {
  return views::AsViewClass<BraveTabStrip>(
      BraveBrowserView::GetBrowserViewForBrowser(browser)
          ->horizontal_tab_strip_for_testing());
}

}  // namespace

class BraveTabStripGetTabMinWidthModeBrowserTest : public InProcessBrowserTest {
 public:
  BraveTabStripGetTabMinWidthModeBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(tabs::kBraveScrollableTabStrip);
  }

  PrefService* prefs() { return browser()->GetProfile()->GetPrefs(); }

  BraveTabStrip* tab_strip() { return GetBraveTabStrip(browser()); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveTabStripGetTabMinWidthModeBrowserTest,
                       GetTabMinWidthMode_ReturnsDefaultWhenScrollablePrefOff) {
  PrefService* profile_prefs = prefs();
  profile_prefs->SetBoolean(brave_tabs::kScrollableHorizontalTabStrip, false);
  profile_prefs->SetInteger(
      brave_tabs::kTabMinWidthMode,
      static_cast<int>(brave_tabs::TabMinWidthMode::kLarge));

  EXPECT_EQ(brave_tabs::TabMinWidthMode::kDefault,
            tab_strip()->GetTabMinWidthMode());
}

IN_PROC_BROWSER_TEST_F(
    BraveTabStripGetTabMinWidthModeBrowserTest,
    GetTabMinWidthMode_ReturnsPrefValueWhenScrollablePrefOn) {
  PrefService* profile_prefs = prefs();
  profile_prefs->SetBoolean(brave_tabs::kScrollableHorizontalTabStrip, true);
  profile_prefs->SetInteger(
      brave_tabs::kTabMinWidthMode,
      static_cast<int>(brave_tabs::TabMinWidthMode::kMedium));

  EXPECT_EQ(brave_tabs::TabMinWidthMode::kMedium,
            tab_strip()->GetTabMinWidthMode());
}

class BraveTabStripGetTabMinWidthModeFeatureDisabledBrowserTest
    : public InProcessBrowserTest {
 public:
  BraveTabStripGetTabMinWidthModeFeatureDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(tabs::kBraveScrollableTabStrip);
  }

  PrefService* prefs() { return browser()->GetProfile()->GetPrefs(); }

  BraveTabStrip* tab_strip() { return GetBraveTabStrip(browser()); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(
    BraveTabStripGetTabMinWidthModeFeatureDisabledBrowserTest,
    GetTabMinWidthMode_ReturnsDefaultWhenFeatureDisabled) {
  PrefService* profile_prefs = prefs();
  profile_prefs->SetBoolean(brave_tabs::kScrollableHorizontalTabStrip, true);
  profile_prefs->SetInteger(
      brave_tabs::kTabMinWidthMode,
      static_cast<int>(brave_tabs::TabMinWidthMode::kLarge));

  EXPECT_EQ(brave_tabs::TabMinWidthMode::kDefault,
            tab_strip()->GetTabMinWidthMode());
}

using BraveTabStripCompactModeBrowserTest = InProcessBrowserTest;

// Verifies that toggling the compact pref at runtime resizes the tab strip.
IN_PROC_BROWSER_TEST_F(BraveTabStripCompactModeBrowserTest,
                       ResizesWithCompactModeToggle) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* local_state = g_browser_process->local_state();

  auto tab_strip_height = [&]() {
    return browser_view->tab_strip_view()->height();
  };

  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, false);
  browser_view->InvalidateLayout();
  RunScheduledLayouts();
  const int default_height = tab_strip_height();
  EXPECT_EQ(default_height, GetLayoutConstant(LayoutConstant::kTabStripHeight));

  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, true);
  RunScheduledLayouts();
  const int compact_height = tab_strip_height();
  EXPECT_EQ(compact_height, GetLayoutConstant(LayoutConstant::kTabStripHeight));
  EXPECT_LT(compact_height, default_height);

  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, false);
  RunScheduledLayouts();
  EXPECT_EQ(tab_strip_height(),
            GetLayoutConstant(LayoutConstant::kTabStripHeight));
}

class BraveTabDoubleClickCloseBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {};

IN_PROC_BROWSER_TEST_P(BraveTabDoubleClickCloseBrowserTest,
                       PreferenceControlsDoubleClickClosing) {
  auto* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsEnabled, GetParam());
  ASSERT_EQ(
      GetParam(),
      VerticalTabController::From(browser())->ShouldShowBraveVerticalTabs());

  chrome::AddTabAt(browser(), GURL(), -1, true);
  chrome::AddTabAt(browser(), GURL(), -1, true);
  auto* model = browser()->tab_strip_model();
  ASSERT_EQ(3, model->count());
  auto* surviving_contents = model->GetWebContentsAt(1);
  auto* tab_strip = GetBraveTabStrip(browser());
  ASSERT_TRUE(tab_strip);
  auto* browser_view = BraveBrowserView::GetBrowserViewForBrowser(browser());
  ui::test::EventGenerator events(
      views::GetRootWindow(browser_view->GetWidget()),
      browser_view->GetNativeWindow());
  auto double_click_first_tab = [&]() {
    tab_strip->StopAnimating();
    RunScheduledLayouts();
    events.MoveMouseTo(tab_strip->tab_at(0)->GetBoundsInScreen().CenterPoint());
    events.DoubleClickLeftButton();
  };

  double_click_first_tab();
  EXPECT_EQ(3, model->count());

  model->ActivateTabAt(2);
  prefs->SetBoolean(brave_tabs::kDoubleClickCloseTabEnabled, true);
  double_click_first_tab();
  ASSERT_TRUE(base::test::RunUntil([&]() { return model->count() == 2; }));
  EXPECT_EQ(surviving_contents, model->GetWebContentsAt(0));
}

INSTANTIATE_TEST_SUITE_P(All,
                         BraveTabDoubleClickCloseBrowserTest,
                         testing::Bool());
