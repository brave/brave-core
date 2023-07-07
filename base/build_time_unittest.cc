/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/build_time.h"

#include "base/time/time.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

// Copied from base/build_time_unittest.cc:

TEST(BuildTime, DateLooksValid) {
  base::Time build_time = base::GetBuildTime();
  base::Time::Exploded exploded_build_time;
  build_time.UTCExplode(&exploded_build_time);
  ASSERT_TRUE(exploded_build_time.HasValidValues());

#if !defined(OFFICIAL_BUILD)
  EXPECT_EQ(exploded_build_time.hour, 5);
  EXPECT_EQ(exploded_build_time.minute, 0);
  EXPECT_EQ(exploded_build_time.second, 0);
#endif
}

TEST(BuildTime, InThePast) {
  EXPECT_LT(base::GetBuildTime(), base::Time::Now());
  EXPECT_LT(base::GetBuildTime(), base::Time::NowFromSystemTime());
}

// Brave-specific tests:

TEST(BuildTime, TimestampIsNotZero) {
  EXPECT_NE(base::GetBuildTime(), base::Time::FromTimeT(0));
}
