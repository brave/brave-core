/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_wallet_balance/get_wallet_balance.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetWalletBalanceTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetWalletBalanceTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetWalletBalance> balance_;

  GetWalletBalanceTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    balance_ = std::make_unique<GetWalletBalance>(mock_ledger_impl_.get());
  }
};

TEST_F(GetWalletBalanceTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "total": 5.0,
              "spendable": 0.0,
              "confirmed": 5.0,
              "unconfirmed": 0.0
            })";
            callback(response);
          }));

  balance_->Request(
      [](const type::Result result, type::BalancePtr balance) {
        type::Balance expected_balance;
        expected_balance.total = 5;
        expected_balance.user_funds = 5;
        expected_balance.wallets = {{constant::kWalletAnonymous, 5}};
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_TRUE(expected_balance.Equals(*balance));
      });
}

TEST_F(GetWalletBalanceTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request(
      [](const type::Result result, type::BalancePtr balance) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!balance);
      });
}

TEST_F(GetWalletBalanceTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request(
      [](const type::Result result, type::BalancePtr balance) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!balance);
      });
}

TEST_F(GetWalletBalanceTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request(
      [](const type::Result result, type::BalancePtr balance) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!balance);
      });
}

TEST_F(GetWalletBalanceTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request(
      [](const type::Result result, type::BalancePtr balance) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_TRUE(!balance);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
