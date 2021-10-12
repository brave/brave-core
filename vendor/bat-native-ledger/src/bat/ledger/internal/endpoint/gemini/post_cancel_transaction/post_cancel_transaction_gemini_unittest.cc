/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/gemini/post_cancel_transaction/post_cancel_transaction_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostCancelTransactionTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostCancelTransactionTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostCancelTransaction> post_cancel_transaction_;

  GeminiPostCancelTransactionTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    post_cancel_transaction_ =
        std::make_unique<PostCancelTransaction>(mock_ledger_impl_.get());
  }
};

TEST_F(GeminiPostCancelTransactionTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_cancel_transaction_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "A5721BF3-530C-42AF-8DEE-005DCFF76970", [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
      });
}

TEST_F(GeminiPostCancelTransactionTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_cancel_transaction_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "A5721BF3-530C-42AF-8DEE-005DCFF76970", [](const type::Result result) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
      });
}

TEST_F(GeminiPostCancelTransactionTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_cancel_transaction_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "A5721BF3-530C-42AF-8DEE-005DCFF76970", [](const type::Result result) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
      });
}

TEST_F(GeminiPostCancelTransactionTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_BAD_REQUEST;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_cancel_transaction_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "A5721BF3-530C-42AF-8DEE-005DCFF76970", [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(GeminiPostCancelTransactionTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_cancel_transaction_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "A5721BF3-530C-42AF-8DEE-005DCFF76970", [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
