/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=UnblindedTest.*

using ::testing::_;

namespace braveledger_contribution {

class UnblindedTest : public ::testing::Test {
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

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(UnblindedTest, PromotionExpiredDeleteTokens) {
//  EXPECT_CALL(*mock_ledger_impl_, OnReconcileComplete(_, _, _, _));
//  ON_CALL(*mock_ledger_impl_, GetAllUnblindedTokens(_))
//      .WillByDefault(
//        testing::Invoke([](ledger::GetAllUnblindedTokensCallback callback) {
//          std::cout << "NEJC 3";
//        }));

  EXPECT_CALL(*mock_ledger_client_, GetAllUnblindedTokens(_));

  unblinded_->Start("some_id");
}

}  // namespace braveledger_contribution
