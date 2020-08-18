/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_clobbered_claims/post_clobbered_claims.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostClobberedClaimsTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostClobberedClaimsTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostClobberedClaims> claims_;

  PostClobberedClaimsTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    claims_ = std::make_unique<PostClobberedClaims>(mock_ledger_impl_.get());
  }
};

TEST_F(PostClobberedClaimsTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 200;
            response.url = url;
            response.body = R"({})";
            callback(response);
          }));

  base::Value corrupted_claims(base::Value::Type::LIST);
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(
      std::move(corrupted_claims),
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_OK);
      });
}

TEST_F(PostClobberedClaimsTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 400;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  base::Value corrupted_claims(base::Value::Type::LIST);
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(
      std::move(corrupted_claims),
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostClobberedClaimsTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 500;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  base::Value corrupted_claims(base::Value::Type::LIST);
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(
      std::move(corrupted_claims),
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PostClobberedClaimsTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _, _, _, _, _))
      .WillByDefault(
          Invoke([](
              const std::string& url,
              const std::vector<std::string>& headers,
              const std::string& content,
              const std::string& contentType,
              const ledger::UrlMethod method,
              ledger::LoadURLCallback callback) {
            ledger::UrlResponse response;
            response.status_code = 453;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  base::Value corrupted_claims(base::Value::Type::LIST);
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(
      std::move(corrupted_claims),
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
