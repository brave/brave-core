/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/report_balance_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ReportBalanceStateTest.*

namespace ledger {

TEST(ReportBalanceStateTest, ToJsonSerialization) {
  // Arrange
  ReportBalanceProperties report_balance_properties;
  report_balance_properties.grants = 1;
  report_balance_properties.ad_earnings = 1;
  report_balance_properties.auto_contributions = 1;
  report_balance_properties.recurring_donations = 1;
  report_balance_properties.one_time_donations = 1;

  // Act
  const ReportBalanceState report_balance_state;
  const std::string json =
      report_balance_state.ToJson(report_balance_properties);

  // Assert
  ReportBalanceProperties expected_report_balance_properties;
  report_balance_state.FromJson(json, &expected_report_balance_properties);
  EXPECT_EQ(expected_report_balance_properties, report_balance_properties);
}

TEST(ReportBalanceStateTest, FromJsonDeserialization) {
  // Arrange
  ReportBalanceProperties report_balance;
  report_balance.grants = 1;
  report_balance.ad_earnings = 1;
  report_balance.auto_contributions = 1;
  report_balance.recurring_donations = 1;
  report_balance.one_time_donations = 1;

  const std::string json = "{\"grants\":1,\"earning_from_ads\":1,\"auto_contribute\":1,\"recurring_donation\":1,\"one_time_donation\":1}";  // NOLINT

  // Act
  ReportBalanceProperties expected_report_balance;
  const ReportBalanceState report_balance_state;
  report_balance_state.FromJson(json, &expected_report_balance);

  // Assert
  EXPECT_EQ(expected_report_balance, report_balance);
}

}  // namespace ledger
