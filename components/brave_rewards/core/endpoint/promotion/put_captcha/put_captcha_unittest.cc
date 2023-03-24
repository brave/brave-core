/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/put_captcha/put_captcha.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PutCaptchaTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PutCaptchaTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PutCaptcha captcha_{&mock_ledger_impl_};
};

TEST_F(PutCaptchaTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  captcha_.Request(10, 20, "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                   base::BindOnce([](mojom::Result result) {
                     EXPECT_EQ(result, mojom::Result::LEDGER_OK);
                   }));
}

TEST_F(PutCaptchaTest, ServerError400) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  captcha_.Request(10, 20, "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                   base::BindOnce([](mojom::Result result) {
                     EXPECT_EQ(result, mojom::Result::CAPTCHA_FAILED);
                   }));
}

TEST_F(PutCaptchaTest, ServerError401) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  captcha_.Request(10, 20, "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                   base::BindOnce([](mojom::Result result) {
                     EXPECT_EQ(result, mojom::Result::CAPTCHA_FAILED);
                   }));
}

TEST_F(PutCaptchaTest, ServerError500) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  captcha_.Request(10, 20, "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                   base::BindOnce([](mojom::Result result) {
                     EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                   }));
}

TEST_F(PutCaptchaTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  captcha_.Request(10, 20, "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                   base::BindOnce([](mojom::Result result) {
                     EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                   }));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
