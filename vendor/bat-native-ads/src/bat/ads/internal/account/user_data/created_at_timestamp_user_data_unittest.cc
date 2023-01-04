/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/created_at_timestamp_user_data.h"

#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

class BatAdsCreatedAtTimestampUserDataTest : public UnitTestBase {};

TEST_F(BatAdsCreatedAtTimestampUserDataTest, GetCreatedAtTimestamp) {
  // Arrange
  const base::Time time =
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false);
  AdvanceClockTo(time);

  // Act
  const base::Value::Dict user_data = GetCreatedAtTimestamp(time);

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"createdAtTimestamp":"2020-11-18T12:00:00.000Z"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace ads::user_data
