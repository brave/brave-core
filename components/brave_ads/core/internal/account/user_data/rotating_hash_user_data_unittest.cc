/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/rotating_hash_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::user_data {

class BatAdsRotatingHashUserDataTest : public UnitTestBase {};

TEST_F(BatAdsRotatingHashUserDataTest, GetRotatingHash) {
  // Arrange
  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));

  TransactionInfo transaction;
  transaction.creative_instance_id = kCreativeInstanceId;

  // Act
  const base::Value::Dict user_data = GetRotatingHash(transaction);

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"rotating_hash":"ooLbypB5vcA/kLjWz+w395SGG+s5M8O9IWbj/OlHuR8="})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsRotatingHashUserDataTest, RotatingHashMatchesBeforeNextHour) {
  // Arrange
  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  TransactionInfo transaction;
  transaction.creative_instance_id = kCreativeInstanceId;

  AdvanceClockTo(TimeFromString("2 June 2022 11:000", /*is_local*/ false));
  const base::Value::Dict user_data_before = GetRotatingHash(transaction);

  // Act
  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  const base::Value::Dict user_data_after = GetRotatingHash(transaction);

  // Assert
  EXPECT_EQ(user_data_before, user_data_after);
}

TEST_F(BatAdsRotatingHashUserDataTest, RotatingHashDifferentAfterNextHour) {
  // Arrange
  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  TransactionInfo transaction;
  transaction.creative_instance_id = kCreativeInstanceId;

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));
  const base::Value::Dict user_data_before = GetRotatingHash(transaction);

  // Act
  AdvanceClockBy(base::Hours(1));

  const base::Value::Dict user_data_after = GetRotatingHash(transaction);

  // Assert
  EXPECT_NE(user_data_before, user_data_after);
}

TEST_F(BatAdsRotatingHashUserDataTest,
       RotatingHashDifferentForSameHourNextDay) {
  // Arrange
  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  TransactionInfo transaction;
  transaction.creative_instance_id = kCreativeInstanceId;

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));
  const base::Value::Dict user_data_before = GetRotatingHash(transaction);

  // Act
  AdvanceClockBy(base::Days(1));

  const base::Value::Dict user_data_after = GetRotatingHash(transaction);

  // Assert
  EXPECT_NE(user_data_before, user_data_after);
}

}  // namespace brave_ads::user_data
