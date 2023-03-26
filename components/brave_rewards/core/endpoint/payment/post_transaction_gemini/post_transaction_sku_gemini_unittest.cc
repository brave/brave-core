/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostTransactionGeminiTest.*

using ::testing::_;

namespace ledger {
namespace endpoint {
namespace payment {

class PostTransactionGeminiTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostTransactionGemini order_{&mock_ledger_impl_};
};

TEST_F(PostTransactionGeminiTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_CREATED;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_.Request(transaction, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_OK);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError400) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_BAD_REQUEST;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_.Request(transaction, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError404) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_NOT_FOUND;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_.Request(transaction, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::NOT_FOUND);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError409) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_CONFLICT;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_.Request(transaction, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

TEST_F(PostTransactionGeminiTest, ServerError500) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_INTERNAL_SERVER_ERROR;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_.Request(transaction, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

TEST_F(PostTransactionGeminiTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = 418;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  mojom::SKUTransaction transaction;
  transaction.order_id = "f2e6494e-fb21-44d1-90e9-b5408799acd8";
  transaction.external_transaction_id = "d382d3ae-8462-4b2c-9b60-b669539f41b2";

  order_.Request(transaction, [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
