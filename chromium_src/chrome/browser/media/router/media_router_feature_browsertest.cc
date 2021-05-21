/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/feature_switch.h"

using extensions::FeatureSwitch;

class MediaRouterTest : public InProcessBrowserTest {
 protected:
  MediaRouterTest() {}
  ~MediaRouterTest() override {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
  }
};

IN_PROC_BROWSER_TEST_F(MediaRouterTest, MediaRouterDefaults) {
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kEnableMediaRouter));
  EXPECT_FALSE(
      FeatureSwitch::load_media_router_component_extension()->IsEnabled());
}

IN_PROC_BROWSER_TEST_F(MediaRouterTest, MediaRouterEnabled) {
  FeatureSwitch::ScopedOverride load_media_router_component_extension(
      FeatureSwitch::load_media_router_component_extension(), true);
  EXPECT_TRUE(media_router::MediaRouterEnabled(browser()->profile()));
  EXPECT_TRUE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kEnableMediaRouter));
}

IN_PROC_BROWSER_TEST_F(MediaRouterTest, MediaRouterToggle) {
  FeatureSwitch::ScopedOverride enabled_override_(
      FeatureSwitch::load_media_router_component_extension(), true);
  EXPECT_TRUE(media_router::MediaRouterEnabled(browser()->profile()));
  EXPECT_TRUE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kEnableMediaRouter));

  FeatureSwitch::ScopedOverride disabled_override_(
      FeatureSwitch::load_media_router_component_extension(), false);
  EXPECT_FALSE(media_router::MediaRouterEnabled(browser()->profile()));
  EXPECT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(prefs::kEnableMediaRouter));
}

// Test to confirm that setting the pref to false disables MediaRouterEnabled
IN_PROC_BROWSER_TEST_F(MediaRouterTest, MediaRouterDisabled) {
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kEnableMediaRouter,
                                               false);
  EXPECT_FALSE(media_router::MediaRouterEnabled(browser()->profile()));
}
