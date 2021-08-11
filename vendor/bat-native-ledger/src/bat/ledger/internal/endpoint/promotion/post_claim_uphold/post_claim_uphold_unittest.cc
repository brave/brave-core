/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_claim_uphold/post_claim_uphold.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostClaimUpholdTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostClaimUpholdTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostClaimUphold> claim_;

  PostClaimUpholdTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    claim_ = std::make_unique<PostClaimUphold>(mock_ledger_impl_.get());
  }
};

const char kExpectedAddress[] = "address";

TEST_F(PostClaimUpholdTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::LEDGER_OK);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

TEST_F(PostClaimUpholdTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

TEST_F(PostClaimUpholdTest, ServerError403) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 403;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::TOO_MANY_RESULTS);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

TEST_F(PostClaimUpholdTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::NOT_FOUND);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

TEST_F(PostClaimUpholdTest, ServerError409) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 409;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::ALREADY_EXISTS);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

TEST_F(PostClaimUpholdTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

TEST_F(PostClaimUpholdTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [](type::UrlRequestPtr request, client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  claim_->Request(30.0, "address",
                  [](const type::Result result, const std::string& address) {
                    EXPECT_EQ(result, type::Result::LEDGER_ERROR);
                    EXPECT_EQ(address, kExpectedAddress);
                  });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
