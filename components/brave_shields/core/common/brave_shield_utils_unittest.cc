// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);
  registry->RegisterBooleanPref(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, false);
}

}  // namespace

TEST(BraveShieldsUtilsTest, IsAdblockOnlyModeFeatureEnabled) {
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);
    EXPECT_TRUE(IsAdblockOnlyModeFeatureEnabled());
  }
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndDisableFeature(features::kAdblockOnlyMode);
    EXPECT_FALSE(IsAdblockOnlyModeFeatureEnabled());
  }
}

TEST(BraveShieldsUtilsTest, IsBraveShieldsAdBlockOnlyModeEnabled) {
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());

  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  EXPECT_TRUE(IsBraveShieldsAdBlockOnlyModeEnabled(&local_state));

  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  EXPECT_FALSE(IsBraveShieldsAdBlockOnlyModeEnabled(&local_state));
}

TEST(BraveShieldsUtilsTest, SetBraveShieldsAdBlockOnlyModeEnabled) {
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());

  SetBraveShieldsAdBlockOnlyModeEnabled(&local_state, /*enabled=*/true);
  EXPECT_TRUE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));

  SetBraveShieldsAdBlockOnlyModeEnabled(&local_state, /*enabled=*/false);
  EXPECT_FALSE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
}

TEST(BraveShieldsUtilsTest, GetLanguageCodeFromLocaleWithoutRegion) {
  EXPECT_EQ("en", GetLanguageCodeFromLocale("en"));
  EXPECT_EQ("fr", GetLanguageCodeFromLocale("Fr"));
  EXPECT_EQ("de", GetLanguageCodeFromLocale("DE"));
}

TEST(BraveShieldsUtilsTest, GetLanguageCodeFromLocaleWithRegion) {
  EXPECT_EQ("en", GetLanguageCodeFromLocale("en-US"));
  EXPECT_EQ("en", GetLanguageCodeFromLocale("En-US"));
  EXPECT_EQ("fr", GetLanguageCodeFromLocale("FR-FR"));
}

TEST(BraveShieldsUtilsTest, GetLanguageCodeFromLocaleWithMalformedLocales) {
  EXPECT_EQ("", GetLanguageCodeFromLocale(""));
  EXPECT_EQ("en", GetLanguageCodeFromLocale("en-"));
  EXPECT_EQ("", GetLanguageCodeFromLocale("-US"));
}

TEST(BraveShieldsUtilsTest, IsAdblockOnlyModeSupportedForLocale) {
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("en"));
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("en-US"));
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("EN"));
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("EN-US"));
}

TEST(BraveShieldsUtilsTest, IsAdblockOnlyModeNotSupportedForLocale) {
  EXPECT_FALSE(IsAdblockOnlyModeSupportedForLocale("fr"));
  EXPECT_FALSE(IsAdblockOnlyModeSupportedForLocale("de-DE"));
  EXPECT_FALSE(IsAdblockOnlyModeSupportedForLocale("ZH-CN"));
}

TEST(BraveShieldsUtilsTest, ManageAdBlockOnlyModeByLocaleWhenFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kAdblockOnlyMode);

  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());
  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  local_state.SetBoolean(prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
                         false);

  ManageAdBlockOnlyModeByLocale(&local_state, "fr-FR");

  // Prefs should remain unchanged.
  EXPECT_TRUE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  EXPECT_FALSE(local_state.GetBoolean(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale));
}

TEST(BraveShieldsUtilsTest,
     ManageAdBlockOnlyModeByLocaleForUnsupportedLocaleWhenModeEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);

  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());
  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  local_state.SetBoolean(prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
                         false);

  ManageAdBlockOnlyModeByLocale(&local_state, "fr-FR");

  // AdBlock Only mode should be disabled and "was enabled" pref should be
  // set to true.
  EXPECT_FALSE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  EXPECT_TRUE(local_state.GetBoolean(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale));
}

TEST(BraveShieldsUtilsTest,
     ManageAdBlockOnlyModeByLocaleForUnsupportedLocaleWhenModeDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);

  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());
  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  local_state.SetBoolean(prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
                         true);

  ManageAdBlockOnlyModeByLocale(&local_state, "fr-FR");

  // Nothing should change since mode is already disabled.
  EXPECT_FALSE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  EXPECT_TRUE(local_state.GetBoolean(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale));
}

TEST(
    BraveShieldsUtilsTest,
    ManageAdBlockOnlyModeByLocaleForSupportedLocaleWhenModeDisabledButWasEnabledPreviously) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());
  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  local_state.SetBoolean(prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
                         true);

  ManageAdBlockOnlyModeByLocale(&local_state, "en-US");

  // AdBlock Only mode should be re-enabled and was enabled pref should be
  // reset.
  EXPECT_TRUE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  EXPECT_FALSE(local_state.GetBoolean(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale));
}

TEST(
    BraveShieldsUtilsTest,
    ManageAdBlockOnlyModeByLocaleForSupportedLocaleWhenModeDisabledAndWasNotEnabledPreviously) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());

  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  local_state.SetBoolean(prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
                         false);

  ManageAdBlockOnlyModeByLocale(&local_state, "en-US");

  // Nothing should change since it was never enabled for a supported locale.
  EXPECT_FALSE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  EXPECT_FALSE(local_state.GetBoolean(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale));
}

TEST(BraveShieldsUtilsTest,
     ManageAdBlockOnlyModeByLocaleForSupportedLocaleWhenModeEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());
  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  local_state.SetBoolean(prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
                         false);

  ManageAdBlockOnlyModeByLocale(&local_state, "en-US");

  // Nothing should change since mode is already enabled.
  EXPECT_TRUE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
  EXPECT_FALSE(local_state.GetBoolean(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale));
}

}  // namespace brave_shields
