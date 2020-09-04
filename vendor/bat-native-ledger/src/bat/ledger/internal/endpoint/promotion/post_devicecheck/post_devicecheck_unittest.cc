/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_devicecheck/post_devicecheck.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostDevicecheckTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostDevicecheckTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostDevicecheck> devicecheck_;

  PostDevicecheckTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    devicecheck_ =
        std::make_unique<PostDevicecheck>(mock_ledger_impl_.get());
  }
};

TEST_F(PostDevicecheckTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "nonce": "c4645786-052f-402f-8593-56af2f7a21ce"
            })";
            callback(response);
          }));

  devicecheck_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(nonce, "c4645786-052f-402f-8593-56af2f7a21ce");
      });
}

TEST_F(PostDevicecheckTest, ServerError400) {
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

  devicecheck_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      });
}

TEST_F(PostDevicecheckTest, ServerError401) {
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

  devicecheck_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      });
}

TEST_F(PostDevicecheckTest, ServerErrorRandom) {
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

  devicecheck_->Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
