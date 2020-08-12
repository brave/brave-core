/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_bat_loss/post_bat_loss.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostBatLossTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostBatLossTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostBatLoss> loss_;

  PostBatLossTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    loss_ = std::make_unique<PostBatLoss>(mock_ledger_impl_.get());
  }

  void SetUp() override {
    const std::string payment_id = "this_is_id";
    ON_CALL(*mock_ledger_client_, GetStringState(ledger::kStatePaymentId))
      .WillByDefault(testing::Return(payment_id));

    const std::string wallet_passphrase =
        "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg=";
    ON_CALL(*mock_ledger_client_, GetStringState(ledger::kStateRecoverySeed))
      .WillByDefault(testing::Return(wallet_passphrase));
  }
};

TEST_F(PostBatLossTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 200;
            response.url = url;
            response.body = R"({})";
            callback(response);
          }));

  loss_->Request(
      30.0,
      1,
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_OK);
      });
}

TEST_F(PostBatLossTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 500;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  loss_->Request(
      30.0,
      1,
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostBatLossTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 453;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  loss_->Request(
      30.0,
      1,
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
