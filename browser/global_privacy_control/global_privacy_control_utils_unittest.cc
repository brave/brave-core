/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/global_privacy_control/global_privacy_control_utils.h"

#include "brave/components/global_privacy_control/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace global_privacy_control {

class GlobalPrivacyControlUtilsTest : public testing::Test {
 public:
  GlobalPrivacyControlUtilsTest() = default;
  ~GlobalPrivacyControlUtilsTest() override = default;

  sync_preferences::TestingPrefServiceSyncable* GetPrefs() {
    return profile_.GetTestingPrefService();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
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
