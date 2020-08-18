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
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostCreds> creds_;

  PostCredsTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    creds_ = std::make_unique<PostCreds>(mock_ledger_impl_.get());
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

TEST_F(PostCredsTest, ServerOK) {
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
            response.body = R"({
              "claimId": "53714048-9675-419e-baa3-369d85a2facb"
            })";
            callback(response);
          }));

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::LEDGER_OK);
        EXPECT_EQ(claim_id, "53714048-9675-419e-baa3-369d85a2facb");
      });
}

TEST_F(PostCredsTest, ServerError400) {
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
            response.status_code = 400;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerError403) {
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
            response.status_code = 403;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerError409) {
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
            response.status_code = 409;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerError410) {
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
            response.status_code = 410;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::NOT_FOUND);
      });
}

TEST_F(PostCredsTest, ServerError500) {
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

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredsTest, ServerErrorRandom) {
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

  auto creds = std::make_unique<base::ListValue>();
  creds->Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      std::move(creds),
      [](const ledger::Result result, const std::string& claim_id) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
