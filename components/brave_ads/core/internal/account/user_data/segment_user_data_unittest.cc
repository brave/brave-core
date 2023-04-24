/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/segment_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSegmentUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsSegmentUserDataTest, DoNotBuildSegmentUserDataIfEmpty) {
  // Arrange

  // Act
  const base::Value::Dict user_data = BuildSegmentUserData(/*transactions*/ {});

  // Assert
  EXPECT_TRUE(user_data.empty());
}

TEST_F(BraveAdsSegmentUserDataTest, BuildSegmentUserData) {
  // Arrange
  TransactionInfo transaction;
  transaction.segment = "untargeted";

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(R"({"segment":"untargeted"})"),
            BuildSegmentUserData(transaction));
}

}  // namespace brave_ads
