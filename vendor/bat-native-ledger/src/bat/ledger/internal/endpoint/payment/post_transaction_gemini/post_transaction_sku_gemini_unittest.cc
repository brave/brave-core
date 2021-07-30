/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostTransactionGeminiTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace payment {

class PostTransactionGeminiTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostTransactionGemini> order_;

  PostTransactionGeminiTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    order_ = std::make_unique<PostTransactionGemini>(mock_ledger_impl_.get());
  }
};

TEST_F(PostTransactionGeminiTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_CREATED;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  type::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_->Request(transaction, [](const type::Result result) {
    EXPECT_EQ(result, type::Result::LEDGER_OK);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_BAD_REQUEST;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  type::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_->Request(transaction, [](const type::Result result) {
    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  type::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_->Request(transaction, [](const type::Result result) {
    EXPECT_EQ(result, type::Result::NOT_FOUND);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError409) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_CONFLICT;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  type::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_->Request(transaction, [](const type::Result result) {
    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_INTERNAL_SERVER_ERROR;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  type::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_->Request(transaction, [](const type::Result result) {
    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
  });
}

TEST_F(PostTransactionGeminiTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  type::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_->Request(transaction, [](const type::Result result) {
    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
  });
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
