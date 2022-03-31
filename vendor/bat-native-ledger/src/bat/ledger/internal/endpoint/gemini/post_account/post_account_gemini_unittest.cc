/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/gemini/post_account/post_account_gemini.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostAccountGeminiTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace gemini {

class GeminiPostAccountTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostAccount> post_account_;

  GeminiPostAccountTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    post_account_ = std::make_unique<PostAccount>(mock_ledger_impl_.get());
  }
};

TEST_F(GeminiPostAccountTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.url = request->url;
            response.body = R"({
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
            callback(response);
          }));

  post_account_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& linking_info,
         const std::string& user_name) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(linking_info, "mocktoken");
        EXPECT_EQ(user_name, "Test");
      });
}

TEST_F(GeminiPostAccountTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_account_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& linking_info,
         const std::string& user_name) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      });
}

TEST_F(GeminiPostAccountTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_FORBIDDEN;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_account_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& linking_info,
         const std::string& user_name) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      });
}

TEST_F(GeminiPostAccountTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = net::HTTP_NOT_FOUND;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_account_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& linking_info,
         const std::string& user_name) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      });
}

TEST_F(GeminiPostAccountTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 418;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  post_account_->Request(
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& linking_info,
         const std::string& user_name) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(linking_info, "");
        EXPECT_EQ(user_name, "");
      });
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
