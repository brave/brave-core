/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view_utils.h"

namespace {

BraveTabStrip* GetBraveTabStrip(Browser* browser) {
  return views::AsViewClass<BraveTabStrip>(
      static_cast<BraveBrowserView*>(browser->window())
          ->horizontal_tab_strip_for_testing());
}

}  // namespace

class BraveTabStripGetTabMinWidthModeBrowserTest : public InProcessBrowserTest {
 public:
  BraveTabStripGetTabMinWidthModeBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(tabs::kBraveScrollableTabStrip);
  }

  PrefService* prefs() { return browser()->profile()->GetPrefs(); }

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

  PrefService* prefs() { return browser()->profile()->GetPrefs(); }

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
