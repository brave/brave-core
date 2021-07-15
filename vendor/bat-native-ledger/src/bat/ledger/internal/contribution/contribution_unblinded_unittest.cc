/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/cxx17_backports.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/database/database_contribution_info.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/database/database_unblinded_token.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=UnblindedTest.*

using ::testing::_;
using ::testing::Invoke;

namespace {
  const char contribution_id[] = "60770beb-3cfb-4550-a5db-deccafb5c790";
}  // namespace

namespace ledger {
namespace contribution {

class UnblindedTest : public ::testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<Unblinded> unblinded_;
  std::unique_ptr<database::MockDatabase> mock_database_;

  UnblindedTest() {
      mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
      mock_ledger_impl_ = std::make_unique<ledger::MockLedgerImpl>
          (mock_ledger_client_.get());
      unblinded_ = std::make_unique<Unblinded>(mock_ledger_impl_.get());
      mock_database_ = std::make_unique<database::MockDatabase>(
        mock_ledger_impl_.get());
  }

  void SetUp() override {
    ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(testing::Return(mock_database_.get()));

    ON_CALL(*mock_database_, GetContributionInfo(contribution_id, _))
    .WillByDefault(
      Invoke([](
          const std::string& id,
          database::GetContributionInfoCallback callback) {
        auto info = type::ContributionInfo::New();
        info->contribution_id = contribution_id;
        info->amount = 5.0;
        info->type = type::RewardsType::ONE_TIME_TIP;
        info->step = type::ContributionStep::STEP_NO;
        info->retry_count = 0;

        callback(std::move(info));
      }));
  }
};

TEST_F(UnblindedTest, NotEnoughFunds) {
  ON_CALL(*mock_database_, GetReservedUnblindedTokens(_, _))
    .WillByDefault(
      Invoke([](
          const std::string&,
          database::GetUnblindedTokenListCallback callback) {
        type::UnblindedTokenList list;

        auto info = type::UnblindedToken::New();
        info->id = 1;
        info->token_value = "asdfasdfasdfsad=";
        info->value = 2;
        info->expires_at = 1574133178;
        list.push_back(info->Clone());

        callback(std::move(list));
      }));

  unblinded_->Start(
      {type::CredsBatchType::PROMOTION},
      contribution_id,
      [](const type::Result result) {
        ASSERT_EQ(result, type::Result::NOT_ENOUGH_FUNDS);
      });
}

TEST_F(UnblindedTest, GetStatisticalVotingWinner) {
  ledger::type::ContributionPublisherList publisher_list;

  auto publisher1 = type::ContributionPublisher::New();
  publisher1->publisher_key = "publisher1";
  publisher1->total_amount = 2.0;
  publisher_list.push_back(std::move(publisher1));

  auto publisher2 = type::ContributionPublisher::New();
  publisher2->publisher_key = "publisher2";
  publisher2->total_amount = 13.0;
  publisher_list.push_back(std::move(publisher2));

  auto publisher3 = type::ContributionPublisher::New();
  publisher3->publisher_key = "publisher3";
  publisher3->total_amount = 14.0;
  publisher_list.push_back(std::move(publisher3));

  auto publisher4 = type::ContributionPublisher::New();
  publisher4->publisher_key = "publisher4";
  publisher4->total_amount = 23.0;
  publisher_list.push_back(std::move(publisher4));

  auto publisher5 = type::ContributionPublisher::New();
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

  for (size_t i = 0; i < base::size(cases); i++) {
    const std::string publisher_key =
        unblinded_->GetStatisticalVotingWinnerForTesting(cases[i].dart, 100.0,
                                                         publisher_list);
    EXPECT_STREQ(publisher_key.c_str(), cases[i].publisher_key);
  }
}

}  // namespace contribution
}  // namespace ledger
