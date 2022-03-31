/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/gemini/post_balance/post_balance_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostBalanceGeminiTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostBalanceTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostBalance> balance_;

  GeminiPostBalanceTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    balance_ = std::make_unique<PostBalance>(mock_ledger_impl_.get());
  }
};

TEST_F(GeminiPostBalanceTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"([
              {
                  "type": "exchange",
                  "currency": "BTC",
                  "amount": "1000.01195318",
                  "available": "1000.01195318",
                  "availableForWithdrawal": "1000.01195318"
              },
              {
                  "type": "exchange",
                  "currency": "ETH",
                  "amount": "20000",
                  "available": "20000",
                  "availableForWithdrawal": "20000"
              },
              {
                  "type": "exchange",
                  "currency": "BAT",
                  "amount": "5000",
                  "available": "5000",
                  "availableForWithdrawal": "5000"
              },
              {
                  "type": "exchange",
                  "currency": "USD",
                  "amount": "93687.50",
                  "available": "93677.40",
                  "availableForWithdrawal": "93677.40"
              }
            ])";
            callback(response);
          }));

  balance_->Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                    [](const type::Result result, const double available) {
                      EXPECT_EQ(result, type::Result::LEDGER_OK);
                      EXPECT_EQ(available, 5000.0);
                    });
}

TEST_F(GeminiPostBalanceTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
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

TEST_F(GeminiPostBalanceTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
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

TEST_F(GeminiPostBalanceTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  balance_->Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                    [](const type::Result result, const double available) {
                      EXPECT_EQ(result, type::Result::NOT_FOUND);
                      EXPECT_EQ(available, 0.0);
                    });
}

TEST_F(GeminiPostBalanceTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
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

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
