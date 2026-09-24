// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/brave_tab_prefs.h"

#include "base/command_line.h"
#include "base/test/scoped_command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/public/switches.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
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

TEST(BraveTabPrefsTest, AlwaysUseMiniAccentIconDefaultsToFalse) {
  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());

  EXPECT_FALSE(prefs.GetBoolean(brave_tabs::kAlwaysUseMiniAccentIcon));
}

TEST(BraveTabPrefsTest,
     MaybeApplyVerticalTabMigrationTestingOverride_NoSwitch_UsesDefault) {
  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());
  prefs.registry()->RegisterBooleanPref(prefs::kVerticalTabsEnabled, false);

  // Set a non-default value for the pref
  prefs.SetBoolean(prefs::kVerticalTabsEnabled, true);
  EXPECT_TRUE(prefs.GetBoolean(prefs::kVerticalTabsEnabled));

  // No switch present - should not touch the pref
  brave_tabs::MaybeApplyVerticalTabMigrationTestingOverride(&prefs);

  // Pref should remain unchanged
  EXPECT_TRUE(prefs.GetBoolean(prefs::kVerticalTabsEnabled));
}

TEST(BraveTabPrefsTest,
     MaybeApplyVerticalTabMigrationTestingOverride_ForceUpstream_SetsPref) {
  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());
  prefs.registry()->RegisterBooleanPref(prefs::kVerticalTabsEnabled, false);

  // Set up the switch
  base::test::ScopedCommandLine cmd_line;
  cmd_line.GetProcessCommandLine()->AppendSwitchASCII(
      tabs::switches::kVerticalTabMigrationSwitch,
      tabs::switches::kVerticalTabMigrationForceUpstreamValue);

  // Apply the override
  brave_tabs::MaybeApplyVerticalTabMigrationTestingOverride(&prefs);

  // Pref should now be true
  EXPECT_TRUE(prefs.GetBoolean(prefs::kVerticalTabsEnabled));
}

TEST(BraveTabPrefsTest,
     MaybeApplyVerticalTabMigrationTestingOverride_Reset_ClearsPref) {
  TestingPrefServiceSimple prefs;
  brave_tabs::RegisterBraveProfilePrefs(prefs.registry());
  prefs.registry()->RegisterBooleanPref(prefs::kVerticalTabsEnabled, false);

  // Set to true explicitly
  prefs.SetBoolean(prefs::kVerticalTabsEnabled, true);
  EXPECT_TRUE(prefs.GetBoolean(prefs::kVerticalTabsEnabled));

  // Set up the reset switch
  base::test::ScopedCommandLine cmd_line;
  cmd_line.GetProcessCommandLine()->AppendSwitchASCII(
      tabs::switches::kVerticalTabMigrationSwitch,
      tabs::switches::kVerticalTabMigrationResetValue);

  // Apply the override
  brave_tabs::MaybeApplyVerticalTabMigrationTestingOverride(&prefs);

  // Pref should be cleared (back to default false)
  EXPECT_FALSE(prefs.GetBoolean(prefs::kVerticalTabsEnabled));
}
