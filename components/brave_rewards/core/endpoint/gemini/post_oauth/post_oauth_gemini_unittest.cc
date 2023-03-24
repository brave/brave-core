/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/post_oauth/post_oauth_gemini.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostOauthTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostOauthTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostOauth oauth_{&mock_ledger_impl_};
};

TEST_F(GeminiPostOauthTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
             "access_token": "aaaaa",
             "expires_in": 83370,
             "scope": "sample:scope",
             "refresh_token":"bbbbb",
             "token_type": "Bearer"
            })";
            std::move(callback).Run(response);
          }));

  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890",
      base::BindOnce([](mojom::Result result, std::string&& token) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(token, "aaaaa");
      }));
}

TEST_F(GeminiPostOauthTest, ServerError401) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890",
      base::BindOnce([](mojom::Result result, std::string&& token) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(token, "");
      }));
}

TEST_F(GeminiPostOauthTest, ServerError403) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890",
      base::BindOnce([](mojom::Result result, std::string&& token) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(token, "");
      }));
}

TEST_F(GeminiPostOauthTest, ServerError404) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890",
      base::BindOnce([](mojom::Result result, std::string&& token) {
        EXPECT_EQ(result, mojom::Result::NOT_FOUND);
        EXPECT_EQ(token, "");
      }));
}

TEST_F(GeminiPostOauthTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  oauth_.Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "1234567890",
      base::BindOnce([](mojom::Result result, std::string&& token) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(token, "");
      }));
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
