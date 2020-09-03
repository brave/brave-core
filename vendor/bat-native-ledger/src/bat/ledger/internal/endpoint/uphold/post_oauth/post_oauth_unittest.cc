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
#include "bat/ledger/internal/endpoint/uphold/post_oauth/post_oauth.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostOauthTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace uphold {

class PostOauthTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostOauth> oauth_;

  PostOauthTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    oauth_ = std::make_unique<PostOauth>(mock_ledger_impl_.get());
  }
};

TEST_F(PostOauthTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
             "access_token": "edc8b465fe2e2a26ce553d937ccc6c7195e9f909",
             "token_type": "bearer",
             "expires_in": 7775999,
             "scope": "accounts:read accounts:write cards:read cards:write"
            })";
            callback(response);
          }));

  oauth_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(token, "edc8b465fe2e2a26ce553d937ccc6c7195e9f909");
      });
}

TEST_F(PostOauthTest, ServerError401) {
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

  oauth_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(token, "");
      });
}

TEST_F(PostOauthTest, ServerErrorRandom) {
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

  oauth_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(token, "");
      });
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
