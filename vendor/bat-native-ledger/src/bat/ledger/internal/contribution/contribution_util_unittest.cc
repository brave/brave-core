/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ContributionUtilTest.*

namespace ledger {
namespace contribution {

class ContributionUtilTest : public testing::Test {};

TEST(ContributionUtilTest, GetReportTypeFromRewardsType) {
  ASSERT_EQ(type::ReportType::AUTO_CONTRIBUTION,
            GetReportTypeFromRewardsType(type::RewardsType::AUTO_CONTRIBUTE));
  ASSERT_EQ(type::ReportType::TIP,
            GetReportTypeFromRewardsType(type::RewardsType::ONE_TIME_TIP));
  ASSERT_EQ(type::ReportType::TIP_RECURRING,
            GetReportTypeFromRewardsType(type::RewardsType::RECURRING_TIP));
}

TEST(ContributionUtilTest, GetProcessor) {
  ASSERT_EQ(type::ContributionProcessor::BRAVE_TOKENS,
            GetProcessor(constant::kWalletUnBlinded));
  ASSERT_EQ(type::ContributionProcessor::UPHOLD,
            GetProcessor(constant::kWalletUphold));
  ASSERT_EQ(type::ContributionProcessor::BITFLYER,
            GetProcessor(constant::kWalletBitflyer));
  ASSERT_EQ(type::ContributionProcessor::GEMINI,
            GetProcessor(constant::kWalletGemini));
  ASSERT_EQ(type::ContributionProcessor::NONE, GetProcessor("random-data"));
}

TEST(ContributionUtilTest, GetNextProcessor) {
  ASSERT_EQ(constant::kWalletUphold,
            GetNextProcessor(constant::kWalletUnBlinded));
  ASSERT_EQ(constant::kWalletBitflyer,
            GetNextProcessor(constant::kWalletUphold));
  ASSERT_EQ(constant::kWalletGemini,
            GetNextProcessor(constant::kWalletBitflyer));
  ASSERT_EQ("", GetNextProcessor(constant::kWalletGemini));
  ASSERT_EQ(constant::kWalletUnBlinded, GetNextProcessor("random-data"));
}

TEST(ContributionUtilTest, HaveEnoughFundsToContribute) {
  double amount = 20.0;
  double balance = 0;
  ASSERT_FALSE(HaveEnoughFundsToContribute(&amount, true, balance));

  balance = 10.0;
  ASSERT_TRUE(HaveEnoughFundsToContribute(&amount, true, balance));
  ASSERT_EQ(amount, 10.0);

  amount = 20.0;
  ASSERT_FALSE(HaveEnoughFundsToContribute(&amount, false, balance));

  amount = 5.0;
  ASSERT_TRUE(HaveEnoughFundsToContribute(&amount, false, balance));
}

}  // namespace contribution
}  // namespace ledger
