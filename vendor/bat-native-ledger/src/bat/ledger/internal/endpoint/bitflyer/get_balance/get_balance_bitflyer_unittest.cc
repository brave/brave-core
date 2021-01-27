/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/bitflyer/get_balance/get_balance_bitflyer.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetBalanceTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace bitflyer {

class GetBalanceTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetBalance> balance_;

  GetBalanceTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    balance_ = std::make_unique<GetBalance>(mock_ledger_impl_.get());
  }
};

TEST_F(GetBalanceTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "account_hash": "ad0fd9160be16790893ff021b2f9ccf7f14b5a9f",
              "inventory": [
                {
                  "currency_code": "JPY",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "BTC",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "BCH",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "ETH",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "ETC",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "LTC",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "MONA",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "LSK",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "XRP",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "BAT",
                  "amount": 4.0,
                  "available": 4.0
                },
                {
                  "currency_code": "XLM",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "XEM",
                  "amount": 0.0,
                  "available": 0.0
                },
                {
                  "currency_code": "XTZ",
                  "amount": 0.0,
                  "available": 0.0
                }
              ]
            })";
            callback(response);
          }));

  balance_->Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                    [](const type::Result result, const double available) {
                      EXPECT_EQ(result, type::Result::LEDGER_OK);
                      EXPECT_EQ(available, 4.0);
                    });
}

TEST_F(GetBalanceTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                    [](const type::Result result, const double available) {
                      EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
                      EXPECT_EQ(available, 0.0);
                    });
}

TEST_F(GetBalanceTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                    [](const type::Result result, const double available) {
                      EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                      EXPECT_EQ(available, 0.0);
                    });
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
