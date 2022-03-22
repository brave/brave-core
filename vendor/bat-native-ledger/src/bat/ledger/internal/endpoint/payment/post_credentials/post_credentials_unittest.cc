/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/payment/post_credentials/post_credentials.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCredentialsTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace payment {

class PostCredentialsTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostCredentials> creds_;

  PostCredentialsTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    creds_ = std::make_unique<PostCredentials>(mock_ledger_impl_.get());
  }
};

TEST_F(PostCredentialsTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "pl2okf23-f2f02kf2fm2-msdkfsodkfds",
      "ff50981d-47de-4210-848d-995e186901a1",
      "single-use",
      std::move(blinded),
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
      });
}

TEST_F(PostCredentialsTest, ServerError400) {
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

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "pl2okf23-f2f02kf2fm2-msdkfsodkfds",
      "ff50981d-47de-4210-848d-995e186901a1",
      "single-use",
      std::move(blinded),
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredentialsTest, ServerError409) {
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

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "pl2okf23-f2f02kf2fm2-msdkfsodkfds",
      "ff50981d-47de-4210-848d-995e186901a1",
      "single-use",
      std::move(blinded),
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredentialsTest, ServerError500) {
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

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "pl2okf23-f2f02kf2fm2-msdkfsodkfds",
      "ff50981d-47de-4210-848d-995e186901a1",
      "single-use",
      std::move(blinded),
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCredentialsTest, ServerErrorRandom) {
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

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_->Request(
      "pl2okf23-f2f02kf2fm2-msdkfsodkfds",
      "ff50981d-47de-4210-848d-995e186901a1",
      "single-use",
      std::move(blinded),
      [](const type::Result result) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
