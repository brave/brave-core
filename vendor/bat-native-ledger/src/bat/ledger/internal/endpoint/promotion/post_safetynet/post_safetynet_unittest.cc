/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_safetynet/post_safetynet.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostSafetynetTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostSafetynetTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostSafetynet> safetynet_;

  PostSafetynetTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    safetynet_ =
        std::make_unique<PostSafetynet>(mock_ledger_impl_.get());
  }
};

TEST_F(PostSafetynetTest, ServerOK) {
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

  safetynet_->Request(
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(nonce, "c4645786-052f-402f-8593-56af2f7a21ce");
      });
}

TEST_F(PostSafetynetTest, ServerError400) {
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

  safetynet_->Request(
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      });
}

TEST_F(PostSafetynetTest, ServerError401) {
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

  safetynet_->Request(
      [](const type::Result result, const std::string& nonce) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
