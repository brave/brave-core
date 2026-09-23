/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats/first_run_util.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_stats {

class FirstRunUtilTest : public testing::Test {
 public:
  FirstRunUtilTest() = default;
  ~FirstRunUtilTest() override = default;

  void SetUp() override {
    local_state_.registry()->RegisterTimePref(kReferralAndroidFirstRunTimestamp,
                                              {});
  }

 protected:
  TestingPrefServiceSimple local_state_;
};

#if BUILDFLAG(IS_ANDROID)
TEST_F(FirstRunUtilTest, IsFirstRunAndroid) {
  EXPECT_TRUE(IsFirstRun(&local_state_));
  // Subsequent calls should succeed
  EXPECT_TRUE(IsFirstRun(&local_state_));

  // Mocking the start of a new process
  ResetAndroidFirstRunStateForTesting();
  EXPECT_FALSE(IsFirstRun(&local_state_));
  EXPECT_FALSE(IsFirstRun(&local_state_));
}

TEST_F(FirstRunUtilTest, GetFirstRunTimeAndroid) {
  base::Time now = base::Time::Now();
  base::Time first_run_time = GetFirstRunTime(&local_state_);
  EXPECT_GE(first_run_time, now);

  EXPECT_EQ(GetFirstRunTime(&local_state_), first_run_time);
}
#endif  // #BUILDFLAG(IS_ANDROID)

}  // namespace brave_stats
