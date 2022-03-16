/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/gemini/post_transaction/post_transaction_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostTransactionTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostTransactionTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostTransaction> transaction_;

  GeminiPostTransactionTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    transaction_ = std::make_unique<PostTransaction>(mock_ledger_impl_.get());
  }
};

TEST_F(GeminiPostTransactionTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
              "result": "OK",
              "tx_ref": "A5721BF3-530C-42AF-8DEE-005DCFF76970",
              "amount": 1,
              "currency": "BAT",
              "destination": "60bf98d6-d1f8-4d35-8650-8d4570a86b60",
              "status": "Completed",
              "timestampms": 1623171893237
            })";
            callback(response);
          }));

  ::ledger::gemini::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::LEDGER_OK);
                          EXPECT_EQ(id, "A5721BF3-530C-42AF-8DEE-005DCFF76970");
                        });
}

TEST_F(GeminiPostTransactionTest, UnrecognizedStatus) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
              "result": "OK",
              "tx_ref": "A5721BF3-530C-42AF-8DEE-005DCFF76970",
              "amount": 1,
              "currency": "BAT",
              "destination": "60bf98d6-d1f8-4d35-8650-8d4570a86b60",
              "status": "Processing",
              "timestampms": 1623171893237
            })";
            callback(response);
          }));

  ::ledger::gemini::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::RETRY);
                          EXPECT_EQ(id, "A5721BF3-530C-42AF-8DEE-005DCFF76970");
                        });
}

TEST_F(GeminiPostTransactionTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  ::ledger::gemini::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
                          EXPECT_EQ(id, "");
                        });
}

TEST_F(GeminiPostTransactionTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  ::ledger::gemini::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
                          EXPECT_EQ(id, "");
                        });
}

TEST_F(GeminiPostTransactionTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  ::ledger::gemini::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::NOT_FOUND);
                          EXPECT_EQ(id, "");
                        });
}

TEST_F(GeminiPostTransactionTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  ::ledger::gemini::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                          EXPECT_EQ(id, "");
                        });
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
