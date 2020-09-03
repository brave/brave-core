/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_captcha/get_captcha.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetCaptchaTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetCaptchaTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetCaptcha> captcha_;

  GetCaptchaTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    captcha_ = std::make_unique<GetCaptcha>(mock_ledger_impl_.get());
  }
};

TEST_F(GetCaptchaTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body =
                R"(aWphaXNqZGZvaWFzamZvc2FpamZvc2lhZGpmb2lkc2pmbw==)";
            callback(response);
          }));

  captcha_->Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      [](const type::Result result, const std::string& image) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(
            image,
            "data:image/jpeg;base64,YVdwaGFYTnFaR1p2YVdGemFtWnZjMkZwYW"
            "1admMybGhaR3BtYjJsa2MycG1idz09");
      });
}

TEST_F(GetCaptchaTest, ServerError400) {
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

  captcha_->Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      [](const type::Result result, const std::string& image) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(GetCaptchaTest, ServerError404) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 404;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  captcha_->Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      [](const type::Result result, const std::string& image) {
        EXPECT_EQ(result, type::Result::NOT_FOUND);
        EXPECT_EQ(image, "");
      });
}

TEST_F(GetCaptchaTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            callback(response);
          }));

  captcha_->Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      [](const type::Result result, const std::string& image) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(image, "");
      });
}

TEST_F(GetCaptchaTest, ServerErrorRandom) {
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

  captcha_->Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      [](const type::Result result, const std::string& image) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
        EXPECT_EQ(image, "");
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
