/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BitflyerPostOauthTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace bitflyer {

class BitflyerPostOauthTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostOauth> oauth_;

  BitflyerPostOauthTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    oauth_ = std::make_unique<PostOauth>(mock_ledger_impl_.get());
  }
};

TEST_F(BitflyerPostOauthTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
             "access_token": "mock_access_token",
             "refresh_token": "mock_refresh_token",
             "expires_in": 259002,
             "scope": "assets create_deposit_id withdraw_to_deposit_id",
             "account_hash": "ad0fd9160be16790893ff021b2f9ccf7f14b5a9f",
             "token_type": "Bearer",
             "linking_info": "mock_linking_info",
             "deposit_id": "339dc5ff-1167-4d69-8dd8-aa77ccb12d74"
            })";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& token,
         const std::string& address, const std::string& linking_info) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(token, "mock_access_token");
        EXPECT_EQ(address, "339dc5ff-1167-4d69-8dd8-aa77ccb12d74");
        EXPECT_EQ(linking_info, "mock_linking_info");
      });
}

TEST_F(BitflyerPostOauthTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& token,
         const std::string& address, const std::string& linking_info) {
        EXPECT_EQ(result, type::Result::EXPIRED_TOKEN);
        EXPECT_EQ(token, "");
      });
}

TEST_F(BitflyerPostOauthTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  oauth_->Request(
      "46553A9E3D57D70F960EA26D95183D8CBB026283D92CBC7C54665408DA7DF398",
      "4c2b665ca060d912fec5c735c734859a06118cc8",
      [](const type::Result result, const std::string& token,
         const std::string& address, const std::string& linking_info) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(token, "");
      });
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
