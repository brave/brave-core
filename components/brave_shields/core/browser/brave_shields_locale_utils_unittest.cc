// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_locale_utils.h"

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

TEST(BraveShieldsLocaleUtilsTest, GetLanguageCodeFromLocaleWithoutRegion) {
  EXPECT_EQ("en", GetLanguageCodeFromLocale("en"));
  EXPECT_EQ("fr", GetLanguageCodeFromLocale("Fr"));
  EXPECT_EQ("de", GetLanguageCodeFromLocale("DE"));
}

TEST(BraveShieldsLocaleUtilsTest, GetLanguageCodeFromLocaleWithRegion) {
  EXPECT_EQ("en", GetLanguageCodeFromLocale("en-US"));
  EXPECT_EQ("en", GetLanguageCodeFromLocale("En-US"));
  EXPECT_EQ("fr", GetLanguageCodeFromLocale("FR-FR"));
}

TEST(BraveShieldsLocaleUtilsTest,
     GetLanguageCodeFromLocaleWithMalformedLocales) {
  EXPECT_EQ("", GetLanguageCodeFromLocale(""));
  EXPECT_EQ("en", GetLanguageCodeFromLocale("en-"));
  EXPECT_EQ("", GetLanguageCodeFromLocale("-US"));
}

TEST(BraveShieldsLocaleUtilsTest, IsAdblockOnlyModeSupportedForLocale) {
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("en"));
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("en-US"));
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("EN"));
  EXPECT_TRUE(IsAdblockOnlyModeSupportedForLocale("EN-US"));
}

TEST(BraveShieldsLocaleUtilsTest, IsAdblockOnlyModeNotSupportedForLocale) {
  EXPECT_FALSE(IsAdblockOnlyModeSupportedForLocale("fr"));
  EXPECT_FALSE(IsAdblockOnlyModeSupportedForLocale("de-DE"));
  EXPECT_FALSE(IsAdblockOnlyModeSupportedForLocale("ZH-CN"));
}

TEST(BraveShieldsLocaleUtilsTest,
     ManageAdBlockOnlyModeByLocaleWhenFeatureDisabled) {
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

TEST(BraveShieldsLocaleUtilsTest,
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

TEST(BraveShieldsLocaleUtilsTest,
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
    BraveShieldsLocaleUtilsTest,
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
    BraveShieldsLocaleUtilsTest,
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

TEST(BraveShieldsLocaleUtilsTest,
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
