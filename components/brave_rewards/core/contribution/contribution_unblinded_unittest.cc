/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/contribution/contribution_unblinded.h"
#include "brave/components/brave_rewards/core/database/database_contribution_info.h"
#include "brave/components/brave_rewards/core/database/database_unblinded_token.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=UnblindedTest.*

using ::testing::_;

namespace {
const char contribution_id[] = "60770beb-3cfb-4550-a5db-deccafb5c790";
}  // namespace

namespace ledger {
namespace contribution {

class UnblindedTest : public ::testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  Unblinded unblinded_{&mock_ledger_impl_};
};

TEST_F(UnblindedTest, NotEnoughFunds) {
  EXPECT_CALL(*mock_ledger_impl_.mock_database(),
              GetSpendableUnblindedTokensByBatchTypes(_, _))
      .Times(1)
      .WillOnce([](const std::vector<mojom::CredsBatchType>&,
                   database::GetUnblindedTokenListCallback callback) {
        std::vector<mojom::UnblindedTokenPtr> tokens;

        auto token = mojom::UnblindedToken::New();
        token->id = 1;
        token->token_value = "asdfasdfasdfsad=";
        token->value = 2;
        token->expires_at = 1574133178;
        tokens.push_back(std::move(token));

        callback(std::move(tokens));
      });

  EXPECT_CALL(*mock_ledger_impl_.mock_database(),
              GetContributionInfo(contribution_id, _))
      .Times(1)
      .WillOnce([](const std::string& id,
                   database::GetContributionInfoCallback callback) {
        auto info = mojom::ContributionInfo::New();
        info->contribution_id = contribution_id;
        info->amount = 5.0;
        info->type = mojom::RewardsType::ONE_TIME_TIP;
        info->step = mojom::ContributionStep::STEP_NO;
        info->retry_count = 0;

        callback(std::move(info));
      });

  unblinded_.Start({mojom::CredsBatchType::PROMOTION}, contribution_id,
                   [](mojom::Result result) {
                     ASSERT_EQ(result, mojom::Result::NOT_ENOUGH_FUNDS);
                   });

  task_environment_.RunUntilIdle();
}

TEST_F(UnblindedTest, GetStatisticalVotingWinner) {
  std::vector<ledger::mojom::ContributionPublisherPtr> publisher_list;

  auto publisher1 = mojom::ContributionPublisher::New();
  publisher1->publisher_key = "publisher1";
  publisher1->total_amount = 2.0;
  publisher_list.push_back(std::move(publisher1));

  auto publisher2 = mojom::ContributionPublisher::New();
  publisher2->publisher_key = "publisher2";
  publisher2->total_amount = 13.0;
  publisher_list.push_back(std::move(publisher2));

  auto publisher3 = mojom::ContributionPublisher::New();
  publisher3->publisher_key = "publisher3";
  publisher3->total_amount = 14.0;
  publisher_list.push_back(std::move(publisher3));

  auto publisher4 = mojom::ContributionPublisher::New();
  publisher4->publisher_key = "publisher4";
  publisher4->total_amount = 23.0;
  publisher_list.push_back(std::move(publisher4));

  auto publisher5 = mojom::ContributionPublisher::New();
  publisher5->publisher_key = "publisher5";
  publisher5->total_amount = 38.0;
  publisher_list.push_back(std::move(publisher5));

  struct {
    double dart;
    const char* publisher_key;
  } cases[] = {
      {0.01, "publisher1"}, {0.05, "publisher2"}, {0.10, "publisher2"},
      {0.20, "publisher3"}, {0.30, "publisher4"}, {0.40, "publisher4"},
      {0.50, "publisher4"}, {0.60, "publisher5"}, {0.70, "publisher5"},
      {0.80, "publisher5"}, {0.90, "publisher5"},
  };

  for (auto& entry : cases) {
    const std::string publisher_key =
        unblinded_.GetStatisticalVotingWinnerForTesting(entry.dart, 100.0,
                                                        publisher_list);
    EXPECT_STREQ(publisher_key.c_str(), entry.publisher_key);
  }
}

}  // namespace contribution
}  // namespace ledger
