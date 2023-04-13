/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/segment_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::user_data {

class BatAdsSegmentUserDataTest : public UnitTestBase {};

TEST_F(BatAdsSegmentUserDataTest, GetEmptySegment) {
  // Arrange

  // Act
  const base::Value::Dict user_data = GetSegment({});

  // Assert
  const base::Value expected_user_data = base::test::ParseJson("{}");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsSegmentUserDataTest, GetSegment) {
  // Arrange
  TransactionInfo transaction;
  transaction.segment = "untargeted";

  // Act
  const base::Value::Dict user_data = GetSegment(transaction);

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"segment":"untargeted"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace brave_ads::user_data
