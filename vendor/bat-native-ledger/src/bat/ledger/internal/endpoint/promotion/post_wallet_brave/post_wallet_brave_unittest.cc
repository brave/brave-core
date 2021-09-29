/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_wallet_brave/post_wallet_brave.h"

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

// npm run test -- brave_unit_tests --filter=PostWalletBraveTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostWalletBraveTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostWalletBrave> wallet_;

  PostWalletBraveTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    wallet_ = std::make_unique<PostWalletBrave>(mock_ledger_impl_.get());
  }

  void SetUp() override {
    const std::string wallet = R"({
      "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
      "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
    })";
    ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBrave))
        .WillByDefault(testing::Return(wallet));
  }
};

TEST_F(PostWalletBraveTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 201;
            response.url = request->url;
            response.body = R"({
              "paymentId": "37742974-3b80-461a-acfb-937e105e5af4"
            })";
            callback(response);
          }));

  wallet_->Request(
      [](const type::Result result, const std::string& payment_id) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(payment_id, "37742974-3b80-461a-acfb-937e105e5af4");
      });
}

TEST_F(PostWalletBraveTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  wallet_->Request(
      [](const type::Result result, const std::string& payment_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(payment_id, "");
      });
}

TEST_F(PostWalletBraveTest, ServerError503) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 503;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  wallet_->Request(
      [](const type::Result result, const std::string& payment_id) {
        EXPECT_EQ(result, type::Result::BAD_REGISTRATION_RESPONSE);
        EXPECT_EQ(payment_id, "");
      });
}

TEST_F(PostWalletBraveTest, ServerErrorRandom) {
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

  wallet_->Request(
      [](const type::Result result, const std::string& payment_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(payment_id, "");
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
