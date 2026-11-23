/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/global_privacy_control/global_privacy_control_utils.h"

#include "base/values.h"
#include "brave/components/global_privacy_control/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace global_privacy_control {

class GlobalPrivacyControlUtilsTest : public testing::Test {
 public:
  GlobalPrivacyControlUtilsTest() {
    prefs_.registry()->RegisterBooleanPref(kGlobalPrivacyControlEnabled, true);
  }
  ~GlobalPrivacyControlUtilsTest() override = default;

  TestingPrefServiceSimple* GetPrefs() { return &prefs_; }

 private:
  TestingPrefServiceSimple prefs_;
};

TEST_F(GlobalPrivacyControlUtilsTest,
       GlobalPrivacyControlEnabledWithoutPolicyWhenPrefEnabled) {
  GetPrefs()->SetBoolean(kGlobalPrivacyControlEnabled, true);

  EXPECT_TRUE(IsGlobalPrivacyControlEnabled(GetPrefs()));
}

TEST_F(GlobalPrivacyControlUtilsTest,
       GlobalPrivacyControlEnabledWithoutPolicyWhenPrefDisabled) {
  GetPrefs()->SetBoolean(kGlobalPrivacyControlEnabled, false);
  EXPECT_TRUE(IsGlobalPrivacyControlEnabled(GetPrefs()));
}

TEST_F(GlobalPrivacyControlUtilsTest,
       GlobalPrivacyControlEnabledWithPolicyWhenPrefEnabled) {
  GetPrefs()->SetManagedPref(kGlobalPrivacyControlEnabled, base::Value(true));
  EXPECT_TRUE(IsGlobalPrivacyControlEnabled(GetPrefs()));
}

TEST_F(GlobalPrivacyControlUtilsTest,
       GlobalPrivacyControlDisabledWithPolicyWhenPrefDisabled) {
  GetPrefs()->SetManagedPref(kGlobalPrivacyControlEnabled, base::Value(false));
  EXPECT_FALSE(IsGlobalPrivacyControlEnabled(GetPrefs()));
}

}  // namespace global_privacy_control
