/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/get_captcha/get_captcha.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetCaptchaTest.*

using ::testing::_;

namespace ledger {
namespace endpoint {
namespace promotion {

class GetCaptchaTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  GetCaptcha captcha_{&mock_ledger_impl_};
};

TEST_F(GetCaptchaTest, ServerOK) {
  EXPECT_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"(aWphaXNqZGZvaWFzamZvc2FpamZvc2lhZGpmb2lkc2pmbw==)";
        std::move(callback).Run(std::move(response));
      });

  captcha_.Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      base::BindOnce([](mojom::Result result, const std::string& image) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(image,
                  "data:image/jpeg;base64,YVdwaGFYTnFaR1p2YVdGemFtWnZjMkZwYW"
                  "1admMybGhaR3BtYjJsa2MycG1idz09");
      }));

  task_environment_.RunUntilIdle();
}

TEST_F(GetCaptchaTest, ServerError400) {
  EXPECT_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  captcha_.Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      base::BindOnce([](mojom::Result result, const std::string& image) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
      }));

  task_environment_.RunUntilIdle();
}

TEST_F(GetCaptchaTest, ServerError404) {
  EXPECT_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 404;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  captcha_.Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      base::BindOnce([](mojom::Result result, const std::string& image) {
        EXPECT_EQ(result, mojom::Result::NOT_FOUND);
        EXPECT_EQ(image, "");
      }));

  task_environment_.RunUntilIdle();
}

TEST_F(GetCaptchaTest, ServerError500) {
  EXPECT_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  captcha_.Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      base::BindOnce([](mojom::Result result, const std::string& image) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(image, "");
      }));

  task_environment_.RunUntilIdle();
}

TEST_F(GetCaptchaTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_ledger_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  captcha_.Request(
      "d155d2d2-2627-425b-9be8-44ae9f541762",
      base::BindOnce([](mojom::Result result, const std::string& image) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(image, "");
      }));

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
