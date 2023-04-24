/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/created_at_timestamp_user_data.h"

#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreatedAtTimestampUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsCreatedAtTimestampUserDataTest,
       BuildCreatedAtTimestampUserData) {
  // Arrange
  const base::Time time =
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false);
  AdvanceClockTo(time);

  TransactionInfo transaction;
  transaction.created_at = time;

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"({"createdAtTimestamp":"2020-11-18T12:00:00.000Z"})"),
            BuildCreatedAtTimestampUserData(transaction));
}

}  // namespace brave_ads
