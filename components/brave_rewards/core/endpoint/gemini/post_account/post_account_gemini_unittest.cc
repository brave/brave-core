/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/post_account/post_account_gemini.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GeminiPostAccountTest.*

using ::testing::_;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostAccountTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostAccount post_account_{&mock_ledger_impl_};
};

TEST_F(GeminiPostAccountTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_OK;
            response->url = request->url;
            response->body = R"({
              "account": {
                "accountName": "Primary",
                "shortName": "primary",
                "type": "exchange",
                "created": "1619040615242",
                "verificationToken": "mocktoken"
              },
              "users": [{
                "name": "Test",
                "lastSignIn": "2021-04-30T18:46:03.017Z",
                "status": "Active",
                "countryCode": "US",
                "isVerified": true
              }],
              "memo_reference_code": "GEMAPLLV"
            })";
            std::move(callback).Run(std::move(response));
          });

  post_account_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& linking_info,
                        std::string&& user_name) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(linking_info, "mocktoken");
        EXPECT_EQ(user_name, "Test");
      }));
}

TEST_F(GeminiPostAccountTest, ServerError401) {
  ON_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_UNAUTHORIZED;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  post_account_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& linking_info,
                        std::string&& user_name) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      }));
}

TEST_F(GeminiPostAccountTest, ServerError403) {
  ON_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_FORBIDDEN;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  post_account_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& linking_info,
                        std::string&& user_name) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      }));
}

TEST_F(GeminiPostAccountTest, ServerError404) {
  ON_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = net::HTTP_NOT_FOUND;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  post_account_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& linking_info,
                        std::string&& user_name) {
        EXPECT_EQ(result, mojom::Result::NOT_FOUND);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      }));
}

TEST_F(GeminiPostAccountTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .WillByDefault(
          [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            auto response = mojom::UrlResponse::New();
            response->status_code = 418;
            response->url = request->url;
            response->body = "";
            std::move(callback).Run(std::move(response));
          });

  post_account_.Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      base::BindOnce([](mojom::Result result, std::string&& linking_info,
                        std::string&& user_name) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      }));
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
