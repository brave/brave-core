/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UpholdTest.*

using ::testing::_;
using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace uphold {

class UpholdTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<Uphold> uphold_;

  UpholdTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    uphold_ = std::make_unique<Uphold>(mock_ledger_impl_.get());
  }
};

TEST_F(UpholdTest, FetchBalanceConnectedWallet) {
  const std::string wallet = R"({
      "token":"token",
      "address":"address"
      "status":1
    })";
  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletUphold))
      .WillByDefault(testing::Return(wallet));
  EXPECT_CALL(*mock_ledger_client_, LoadURL(_, _)).Times(0);

  FetchBalanceCallback callback =
      std::bind(
          [&](type::Result result, double balance) {
            ASSERT_EQ(result, type::Result::LEDGER_OK);
            ASSERT_EQ(balance, 0.0);
          },
          _1,
          _2);

  uphold_->FetchBalance(callback);
}

}  // namespace uphold
}  // namespace ledger
