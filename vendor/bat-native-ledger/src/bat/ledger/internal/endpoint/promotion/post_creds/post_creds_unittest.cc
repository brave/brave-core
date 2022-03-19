/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_creds/post_creds.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCredsTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostCredsTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostCreds> creds_;

  PostCredsTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    creds_ = std::make_unique<PostCreds>(mock_ledger_impl_.get());
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

TEST_F(PostCredsTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "claimId": "53714048-9675-419e-baa3-369d85a2facb"
            })";
            callback(response);
          }));

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(claim_id, "53714048-9675-419e-baa3-369d85a2facb");
      });
}

TEST_F(PostCredsTest, ServerError400) {
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

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 403;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerError409) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 409;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerError410) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 410;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
      });
}

TEST_F(PostCredsTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerErrorRandom) {
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

  base::Value::List creds;
  creds.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const type::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
