/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_captcha/post_captcha.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCaptchaTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostCaptchaTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostCaptcha> captcha_;

  PostCaptchaTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
    captcha_ =
        std::make_unique<PostCaptcha>(mock_ledger_impl_.get());
  }
};

TEST_F(PostCaptchaTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](
              type::UrlRequestPtr request,
              client::LoadURLCallback callback) {
            type::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "hint": "circle",
              "captchaId": "d155d2d2-2627-425b-9be8-44ae9f541762"
            })";
            callback(response);
          }));

  captcha_->Request(
      [](
          const type::Result result,
          const std::string& hint,
          const std::string& captcha_id) {
        EXPECT_EQ(result, type::Result::LEDGER_OK);
        EXPECT_EQ(hint, "circle");
        EXPECT_EQ(captcha_id, "d155d2d2-2627-425b-9be8-44ae9f541762");
      });
}

TEST_F(PostCaptchaTest, ServerError400) {
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
      [](
          const type::Result result,
          const std::string& hint,
          const std::string& captcha_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

TEST_F(PostCaptchaTest, ServerErrorRandom) {
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
      [](
          const type::Result result,
          const std::string& hint,
          const std::string& captcha_id) {
        EXPECT_EQ(result, type::Result::LEDGER_ERROR);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
