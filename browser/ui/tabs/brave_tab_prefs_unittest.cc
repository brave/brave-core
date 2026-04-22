// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tab_prefs.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveTabPrefsTest,
     IsScrollableHorizontalTabStripEnabled_FalseWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(tabs::kBraveScrollableTabStrip);

  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());
  prefs.SetBoolean(brave_tabs::kScrollableHorizontalTabStrip, true);

  EXPECT_FALSE(brave_tabs::IsScrollableHorizontalTabStripEnabled(&prefs));
}

TEST(BraveTabPrefsTest,
     IsScrollableHorizontalTabStripEnabled_FalseWhenPrefDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(tabs::kBraveScrollableTabStrip);

  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());
  prefs.SetBoolean(brave_tabs::kScrollableHorizontalTabStrip, false);

  EXPECT_FALSE(brave_tabs::IsScrollableHorizontalTabStripEnabled(&prefs));
}

TEST(BraveTabPrefsTest,
     IsScrollableHorizontalTabStripEnabled_TrueWhenFeatureAndPrefOn) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(tabs::kBraveScrollableTabStrip);

  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());
  prefs.SetBoolean(brave_tabs::kScrollableHorizontalTabStrip, true);

  EXPECT_TRUE(brave_tabs::IsScrollableHorizontalTabStripEnabled(&prefs));
}
