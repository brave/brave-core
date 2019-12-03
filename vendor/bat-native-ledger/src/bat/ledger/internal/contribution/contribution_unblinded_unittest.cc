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
    ON_CALL(*mock_ledger_impl_, GetReconcileById(_))
      .WillByDefault(
        Invoke([](const std::string& viewing_id) {
          ledger::CurrentReconcileProperties reconcile;
          reconcile.fee = 5.0;
          return reconcile;
        }));
  }
};

TEST_F(UnblindedTest, NotEnoughFunds) {
  const std::string viewing_id = "some_id";
  EXPECT_CALL(*mock_ledger_impl_,
      ReconcileComplete(ledger::Result::NOT_ENOUGH_FUNDS, _, _, _, _));

  std::vector<std::string> delete_list;
  delete_list.push_back("1");
  EXPECT_CALL(*mock_ledger_impl_, DeleteUnblindedTokens(delete_list, _));

  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
    .WillByDefault(
      Invoke([](ledger::GetAllUnblindedTokensCallback callback) {
        ledger::UnblindedTokenList list;

          auto info = ledger::UnblindedToken::New();
          info->id = 1;
          info->token_value = "asdfasdfasdfsad=";
          info->value = 2;
          info->expires_at = 1574133178;
          list.push_back(info->Clone());

        callback(std::move(list));
      }));

  unblinded_->Start(viewing_id);
}

TEST_F(UnblindedTest, PromotionExpiredDeleteToken) {
  const std::string viewing_id = "some_id";
  EXPECT_CALL(*mock_ledger_impl_,
      ReconcileComplete(ledger::Result::NOT_ENOUGH_FUNDS, _, _, _, _))
      .Times(0);

  std::vector<std::string> delete_list;
  delete_list.push_back("1");
  EXPECT_CALL(*mock_ledger_impl_, DeleteUnblindedTokens(delete_list, _));

  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
      .WillByDefault(
        Invoke([](ledger::GetAllUnblindedTokensCallback callback) {
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

  unblinded_->Start(viewing_id);
}

TEST_F(UnblindedTest, PromotionExpiredDeleteTokensNotEnoughFunds) {
  const std::string viewing_id = "some_id";
  EXPECT_CALL(*mock_ledger_impl_,
      ReconcileComplete(ledger::Result::NOT_ENOUGH_FUNDS, _, _, _, _));

  std::vector<std::string> delete_list;
  delete_list.push_back("1");
  delete_list.push_back("2");
  EXPECT_CALL(*mock_ledger_impl_, DeleteUnblindedTokens(delete_list, _));

  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
      .WillByDefault(
        Invoke([](ledger::GetAllUnblindedTokensCallback callback) {
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

  unblinded_->Start(viewing_id);
}

}  // namespace braveledger_contribution
