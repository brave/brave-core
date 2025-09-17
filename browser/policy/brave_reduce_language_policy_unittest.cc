// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveReduceLanguagePolicyTest : public testing::Test {
 public:
  BraveReduceLanguagePolicyTest() {
    pref_service_.registry()->RegisterBooleanPref(
        brave_shields::prefs::kReduceLanguageEnabled, true);
  }

 protected:
  void SetBraveReduceLanguagePolicyEnabled(bool value) {
    pref_service_.SetManagedPref(brave_shields::prefs::kReduceLanguageEnabled,
                                 base::Value(value));
  }

  void RemoveBraveReduceLanguagePolicy() {
    pref_service_.RemoveManagedPref(
        brave_shields::prefs::kReduceLanguageEnabled);
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveReduceLanguagePolicyTest, PolicyDisablesReduceLanguageDefault) {
  EXPECT_TRUE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
  EXPECT_FALSE(pref_service_.IsManagedPreference(
      brave_shields::prefs::kReduceLanguageEnabled));

  SetBraveReduceLanguagePolicyEnabled(false);

  EXPECT_TRUE(pref_service_.IsManagedPreference(
      brave_shields::prefs::kReduceLanguageEnabled));
  EXPECT_FALSE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
}

TEST_F(BraveReduceLanguagePolicyTest, PolicyDisablesReduceLanguageSetByUser) {
  pref_service_.SetBoolean(brave_shields::prefs::kReduceLanguageEnabled, true);
  EXPECT_FALSE(pref_service_.IsManagedPreference(
      brave_shields::prefs::kReduceLanguageEnabled));

  SetBraveReduceLanguagePolicyEnabled(false);

  EXPECT_TRUE(pref_service_.IsManagedPreference(
      brave_shields::prefs::kReduceLanguageEnabled));
  EXPECT_FALSE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
}

TEST_F(BraveReduceLanguagePolicyTest, PolicyEnablesReduceLanguage) {
  pref_service_.SetBoolean(brave_shields::prefs::kReduceLanguageEnabled, false);
  EXPECT_FALSE(pref_service_.IsManagedPreference(
      brave_shields::prefs::kReduceLanguageEnabled));

  SetBraveReduceLanguagePolicyEnabled(true);

  EXPECT_TRUE(pref_service_.IsManagedPreference(
      brave_shields::prefs::kReduceLanguageEnabled));
  EXPECT_TRUE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
}

TEST_F(BraveReduceLanguagePolicyTest, ReduceLanguageDefaultAfterPolicyRemoved) {
  EXPECT_TRUE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));

  SetBraveReduceLanguagePolicyEnabled(false);

  EXPECT_FALSE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));

  RemoveBraveReduceLanguagePolicy();

  EXPECT_TRUE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
}

TEST_F(BraveReduceLanguagePolicyTest,
       ReduceLanguageUserValueAfterPolicyRemoved) {
  pref_service_.SetBoolean(brave_shields::prefs::kReduceLanguageEnabled, true);

  SetBraveReduceLanguagePolicyEnabled(false);

  EXPECT_FALSE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));

  RemoveBraveReduceLanguagePolicy();

  EXPECT_TRUE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
}

TEST_F(BraveReduceLanguagePolicyTest,
       IgnoreReduceLanguagePrefChangeWhenPolicyIsSet) {
  pref_service_.SetBoolean(brave_shields::prefs::kReduceLanguageEnabled, true);

  SetBraveReduceLanguagePolicyEnabled(false);

  EXPECT_FALSE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));

  pref_service_.SetBoolean(brave_shields::prefs::kReduceLanguageEnabled, true);

  EXPECT_FALSE(
      pref_service_.GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
}
