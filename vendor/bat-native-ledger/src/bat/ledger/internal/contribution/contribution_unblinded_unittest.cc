/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=UnblindedTest.*

using ::testing::_;
using ::testing::Invoke;

namespace {
  const char contribution_id[] = "60770beb-3cfb-4550-a5db-deccafb5c790";
}  // namespace

namespace braveledger_contribution {

class UnblindedTest : public ::testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<Unblinded> unblinded_;

  UnblindedTest() {
      mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
      mock_ledger_impl_ = std::make_unique<bat_ledger::MockLedgerImpl>
          (mock_ledger_client_.get());
      unblinded_ = std::make_unique<Unblinded>(mock_ledger_impl_.get());
  }

  void SetUp() override {
    ON_CALL(*mock_ledger_impl_, GetContributionInfo(contribution_id, _))
    .WillByDefault(
      Invoke([](
          const std::string& id,
          ledger::GetContributionInfoCallback callback) {
        auto info = ledger::ContributionInfo::New();
        info->contribution_id = contribution_id;
        info->amount = 5.0;
        info->type = ledger::RewardsType::ONE_TIME_TIP;
        info->step = ledger::ContributionStep::STEP_NO;
        info->retry_count = 0;

        callback(std::move(info));
      }));
  }
};

TEST_F(UnblindedTest, NotEnoughFunds) {
  std::vector<std::string> delete_list;
  delete_list.push_back("1");
  EXPECT_CALL(*mock_ledger_impl_, DeleteUnblindedTokens(delete_list, _));

  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
    .WillByDefault(
      Invoke([](ledger::GetUnblindedTokenListCallback callback) {
        ledger::UnblindedTokenList list;

        auto info = ledger::UnblindedToken::New();
        info->id = 1;
        info->token_value = "asdfasdfasdfsad=";
        info->value = 2;
        info->expires_at = 1574133178;
        list.push_back(info->Clone());

        callback(std::move(list));
      }));

  unblinded_->Start(contribution_id, [](const ledger::Result result) {
    ASSERT_EQ(result, ledger::Result::NOT_ENOUGH_FUNDS);
  });
}

TEST_F(UnblindedTest, PromotionExpiredDeleteToken) {
  EXPECT_CALL(*mock_ledger_impl_, UpdateContributionInfoStep(
      _,
      ledger::ContributionStep::STEP_PREPARE,
      _));

  std::vector<std::string> delete_list;
  delete_list.push_back("1");
  EXPECT_CALL(*mock_ledger_impl_, DeleteUnblindedTokens(delete_list, _));

  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
      .WillByDefault(
        Invoke([](ledger::GetUnblindedTokenListCallback callback) {
          ledger::UnblindedTokenList list;

          auto info = ledger::UnblindedToken::New();
          info->id = 1;
          info->token_value = "asdfasdfasdfsad=";
          info->value = 5;
          info->expires_at = 1574133178;
          list.push_back(info->Clone());

          info->id = 2;
          info->expires_at = 22574133178;
          list.push_back(info->Clone());

          callback(std::move(list));
        }));

  unblinded_->Start(contribution_id, [](const ledger::Result result) {
    ASSERT_EQ(result, ledger::Result::LEDGER_ERROR);
  });
}

TEST_F(UnblindedTest, PromotionExpiredDeleteTokensNotEnoughFunds) {
  std::vector<std::string> delete_list;
  delete_list.push_back("1");
  delete_list.push_back("2");
  EXPECT_CALL(*mock_ledger_impl_, DeleteUnblindedTokens(delete_list, _));

  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
      .WillByDefault(
        Invoke([](ledger::GetUnblindedTokenListCallback callback) {
          ledger::UnblindedTokenList list;

          auto info = ledger::UnblindedToken::New();
          info->id = 1;
          info->token_value = "asdfasdfasdfsad=";
          info->value = 3;
          info->expires_at = 1574133178;
          list.push_back(info->Clone());

          info->id = 2;
          list.push_back(info->Clone());

          callback(std::move(list));
        }));

  unblinded_->Start(contribution_id, [](const ledger::Result result) {
    ASSERT_EQ(result, ledger::Result::NOT_ENOUGH_FUNDS);
  });
}

}  // namespace braveledger_contribution
