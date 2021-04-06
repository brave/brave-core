/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/bitflyer/post_transaction/post_transaction_bitflyer.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BitflyerPostTransactionTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace bitflyer {

class BitflyerPostTransactionTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostTransaction> transaction_;

  BitflyerPostTransactionTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    transaction_ = std::make_unique<PostTransaction>(mock_ledger_impl_.get());
  }
};

TEST_F(BitflyerPostTransactionTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
             "currency_code": "BAT",
             "amount": "1.00",
             "dry_run": true,
             "message": null,
             "transfer_id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
             "transfer_status": "SUCCESS"
            })";
            callback(response);
          }));

  ::ledger::bitflyer::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        false,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::LEDGER_OK);
                          EXPECT_EQ(id, "d382d3ae-8462-4b2c-9b60-b669539f41b2");
                        });
}

TEST_F(BitflyerPostTransactionTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  ::ledger::bitflyer::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        false,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
                          EXPECT_EQ(id, "");
                        });
}

TEST_F(BitflyerPostTransactionTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  ::ledger::bitflyer::Transaction transaction;
  transaction.amount = 1.0;
  transaction.address = "6654ecb0-6079-4f6c-ba58-791cc890a561";

  transaction_->Request("4c2b665ca060d912fec5c735c734859a06118cc8", transaction,
                        false,
                        [](const type::Result result, const std::string& id) {
                          EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                          EXPECT_EQ(id, "");
                        });
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
