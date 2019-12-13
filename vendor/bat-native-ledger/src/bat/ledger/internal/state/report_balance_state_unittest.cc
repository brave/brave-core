/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/report_balance_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ReportBalanceStateTest.*

namespace ledger {

TEST(ReportBalanceStateTest, ToJsonSerialization) {
  // Arrange
  ReportBalanceProperties report_balance_properties;
  report_balance_properties.grants = "Grants";
  report_balance_properties.ad_earnings = "AdEarnings";
  report_balance_properties.auto_contributions = "AutoContributions";
  report_balance_properties.recurring_donations = "RecurringDonations";
  report_balance_properties.one_time_donations = "OneTimeDonations";

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
  report_balance.grants = "Grants";
  report_balance.ad_earnings = "AdEarnings";
  report_balance.auto_contributions = "AutoContributions";
  report_balance.recurring_donations = "RecurringDonations";
  report_balance.one_time_donations = "OneTimeDonations";

  const std::string json = "{\"grants\":\"Grants\",\"earning_from_ads\":\"AdEarnings\",\"auto_contribute\":\"AutoContributions\",\"recurring_donation\":\"RecurringDonations\",\"one_time_donation\":\"OneTimeDonations\"}";  // NOLINT

  // Act
  ReportBalanceProperties expected_report_balance;
  const ReportBalanceState report_balance_state;
  report_balance_state.FromJson(json, &expected_report_balance);

  // Assert
  EXPECT_EQ(expected_report_balance, report_balance);
}

}  // namespace ledger
