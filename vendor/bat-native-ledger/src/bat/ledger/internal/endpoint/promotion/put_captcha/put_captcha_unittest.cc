/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/put_captcha/put_captcha.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PutCaptchaTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PutCaptchaTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PutCaptcha> captcha_;

  PutCaptchaTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    captcha_ =
        std::make_unique<PutCaptcha>(mock_ledger_impl_.get());
  }
};

TEST_F(PutCaptchaTest, ServerOK) {
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

  captcha_->Request(
      10,
      20,
      "83b3b77b-e7c3-455b-adda-e476fa0656d2",
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_OK);
      });
}

TEST_F(PutCaptchaTest, ServerError400) {
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

  captcha_->Request(
      10,
      20,
      "83b3b77b-e7c3-455b-adda-e476fa0656d2",
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::CAPTCHA_FAILED);
      });
}

TEST_F(PutCaptchaTest, ServerError401) {
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
            response.status_code = 401;
            response.url = url;
            response.body = "";
            callback(response);
          }));

  captcha_->Request(
      10,
      20,
      "83b3b77b-e7c3-455b-adda-e476fa0656d2",
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::CAPTCHA_FAILED);
      });
}

TEST_F(PutCaptchaTest, ServerError500) {
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

  captcha_->Request(
      10,
      20,
      "83b3b77b-e7c3-455b-adda-e476fa0656d2",
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

TEST_F(PutCaptchaTest, ServerErrorRandom) {
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

  captcha_->Request(
      10,
      20,
      "83b3b77b-e7c3-455b-adda-e476fa0656d2",
      [](const ledger::Result result) {
        EXPECT_EQ(result, ledger::Result::LEDGER_ERROR);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
