/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/gemini/post_oauth/post_oauth_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostOauthTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostOauthTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostOauth> oauth_;

  GeminiPostOauthTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    oauth_ = std::make_unique<PostOauth>(mock_ledger_impl_.get());
  }
};

TEST_F(GeminiPostOauthTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
             "access_token": "aaaaa",
             "expires_in": 83370,
             "scope": "sample:scope",
             "refresh_token":"bbbbb",
             "token_type": "Bearer"
            })";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(token, "aaaaa");
      });
}

TEST_F(GeminiPostOauthTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(token, "");
      });
}

TEST_F(GeminiPostOauthTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(token, "");
      });
}

TEST_F(GeminiPostOauthTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_EQ(token, "");
      });
}

TEST_F(GeminiPostOauthTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890", [](const type::Result result, const std::string& token) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(token, "");
      });
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
