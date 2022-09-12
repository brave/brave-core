/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/endpoint/uphold/post_transaction/post_transaction.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostTransactionTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace uphold {

class PostTransactionTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostTransaction> transaction_;

  PostTransactionTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    transaction_ = std::make_unique<PostTransaction>(mock_ledger_impl_.get());
  }
};

TEST_F(PostTransactionTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 202;
            response.url = request->url;
            response.body = R"({
             "createdAt": "2020-06-10T18:58:21.683Z",
             "denomination": {
               "amount": "1.00",
               "currency": "BAT",
               "pair": "BATBAT",
               "rate": "1.00"
             },
             "fees": [],
             "id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
             "network": "uphold",
             "normalized": [
               {
                 "commission": "0.00",
                 "currency": "USD",
                 "fee": "0.00",
                 "rate": "0.24688",
                 "target": "origin",
                 "amount": "0.25"
               }
             ],
             "params": {
               "currency": "BAT",
               "margin": "0.00",
               "pair": "BATBAT",
               "rate": "1.00",
               "ttl": 3599588,
               "type": "internal"
             },
             "priority": "normal",
             "status": "pending",
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
               "sources": [],
               "type": "card"
             }
            })";
            std::move(callback).Run(response);
          }));

  ::ledger::uphold::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("bd91a720-f3f9-42f8-b2f5-19548004f6a7",
                        "4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const mojom::Result result, const std::string& id) {
                          EXPECT_EQ(result, mojom::Result::LEDGER_OK);
                          EXPECT_EQ(id, "d382d3ae-8462-4b2c-9b60-b669539f41b2");
                        });
}

TEST_F(PostTransactionTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  ::ledger::uphold::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("bd91a720-f3f9-42f8-b2f5-19548004f6a7",
                        "4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const mojom::Result result, const std::string& id) {
                          EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
                          EXPECT_EQ(id, "");
                        });
}

TEST_F(PostTransactionTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  ::ledger::uphold::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("bd91a720-f3f9-42f8-b2f5-19548004f6a7",
                        "4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        [](const mojom::Result result, const std::string& id) {
                          EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                          EXPECT_EQ(id, "");
                        });
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
