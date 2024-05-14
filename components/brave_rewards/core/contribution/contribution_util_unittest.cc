/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/contribution/contribution_util.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {
namespace contribution {

class RewardsContributionUtilTest : public testing::Test {};

TEST(RewardsContributionUtilTest, GetReportTypeFromRewardsType) {
  ASSERT_EQ(mojom::ReportType::AUTO_CONTRIBUTION,
            GetReportTypeFromRewardsType(mojom::RewardsType::AUTO_CONTRIBUTE));
  ASSERT_EQ(mojom::ReportType::TIP,
            GetReportTypeFromRewardsType(mojom::RewardsType::ONE_TIME_TIP));
  ASSERT_EQ(mojom::ReportType::TIP_RECURRING,
            GetReportTypeFromRewardsType(mojom::RewardsType::RECURRING_TIP));
}

TEST(RewardsContributionUtilTest, GetProcessor) {
  ASSERT_EQ(mojom::ContributionProcessor::BRAVE_TOKENS,
            GetProcessor(constant::kWalletUnBlinded));
  ASSERT_EQ(mojom::ContributionProcessor::UPHOLD,
            GetProcessor(constant::kWalletUphold));
  ASSERT_EQ(mojom::ContributionProcessor::BITFLYER,
            GetProcessor(constant::kWalletBitflyer));
  ASSERT_EQ(mojom::ContributionProcessor::GEMINI,
            GetProcessor(constant::kWalletGemini));
  ASSERT_EQ(mojom::ContributionProcessor::NONE, GetProcessor("random-data"));
}

TEST(RewardsContributionUtilTest, GetNextProcessor) {
  ASSERT_EQ(constant::kWalletUphold,
            GetNextProcessor(constant::kWalletUnBlinded));
  ASSERT_EQ(constant::kWalletBitflyer,
            GetNextProcessor(constant::kWalletUphold));
  ASSERT_EQ(constant::kWalletGemini,
            GetNextProcessor(constant::kWalletBitflyer));
  ASSERT_EQ("", GetNextProcessor(constant::kWalletGemini));
  ASSERT_EQ(constant::kWalletUnBlinded, GetNextProcessor("random-data"));
}

TEST(RewardsContributionUtilTest, HaveEnoughFundsToContribute) {
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
}  // namespace brave_rewards::internal
