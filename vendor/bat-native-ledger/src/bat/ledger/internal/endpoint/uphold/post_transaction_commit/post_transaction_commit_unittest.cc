/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/endpoint/uphold/post_transaction_commit/post_transaction_commit.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostTransactionCommitTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace uphold {

class PostTransactionCommitTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostTransactionCommit> transaction_;

  PostTransactionCommitTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    transaction_ =
        std::make_unique<PostTransactionCommit>(mock_ledger_impl_.get());
  }
};

TEST_F(PostTransactionCommitTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
             "application": {
               "name": "Brave Browser"
             },
             "createdAt": "2020-06-10T18:58:22.351Z",
             "denomination": {
               "pair": "BATBAT",
               "rate": "1.00",
               "amount": "1.00",
               "currency": "BAT"
             },
             "fees": [],
             "id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
             "message": null,
             "network": "uphold",
             "normalized": [
               {
                 "fee": "0.00",
                 "rate": "0.24688",
                 "amount": "0.25",
                 "target": "origin",
                 "currency": "USD",
                 "commission": "0.00"
               }
             ],
             "params": {
               "currency": "BAT",
               "margin": "0.00",
               "pair": "BATBAT",
               "progress": "1",
               "rate": "1.00",
               "ttl": 3599588,
               "type": "internal"
             },
             "priority": "normal",
             "reference": null,
             "status": "completed",
             "type": "transfer",
             "destination": {
               "amount": "1.00",
               "base": "1.00",
               "commission": "0.00",
               "currency": "BAT",
               "description": "Brave Software International",
               "fee": "0.00",
               "isMember": true,
               "node": {
                 "id": "6654ecb0-6079-4f6c-ba58-791cc890a561",
                 "type": "card",
                 "user": {
                   "id": "f5e37294-68f1-49ae-89e2-b24b64aedd37",
                   "username": "braveintl"
                 }
               },
               "rate": "1.00",
               "type": "card",
               "username": "braveintl"
             },
             "origin": {
               "amount": "1.00",
               "base": "1.00",
               "CardId": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
               "commission": "0.00",
               "currency": "BAT",
               "description": "User",
               "fee": "0.00",
               "isMember": true,
               "node": {
                 "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
                 "type": "card",
                 "user": {
                   "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e"
                 }
               },
               "rate": "1.00",
               "sources": [
                 {
                   "id": "463dca02-83ec-4bd6-93b0-73bf5dbe35ac",
                   "amount": "1.00"
                 }
               ],
               "type": "card"
             }
            })";
            callback(response);
          }));

  transaction_->Request(
      "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "6654ecb0-6079-4f6c-ba58-791cc890a561",
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
      });
}

TEST_F(PostTransactionCommitTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  transaction_->Request(
      "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "6654ecb0-6079-4f6c-ba58-791cc890a561",
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
      });
}

TEST_F(PostTransactionCommitTest, ServerErrorRandom) {
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

  transaction_->Request(
      "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      "6654ecb0-6079-4f6c-ba58-791cc890a561",
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
