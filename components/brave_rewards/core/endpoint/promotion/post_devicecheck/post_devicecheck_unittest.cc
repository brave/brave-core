/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_devicecheck/post_devicecheck.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostDevicecheckTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace promotion {

class PostDevicecheckTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostDevicecheck devicecheck_{&mock_ledger_impl_};
};

TEST_F(PostDevicecheckTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = R"({
              "nonce": "c4645786-052f-402f-8593-56af2f7a21ce"
            })";
            std::move(callback).Run(response);
          }));

  devicecheck_.Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      base::BindOnce([](mojom::Result result, const std::string& nonce) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(nonce, "c4645786-052f-402f-8593-56af2f7a21ce");
      }));
}

TEST_F(PostDevicecheckTest, ServerError400) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_.Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      base::BindOnce([](mojom::Result result, const std::string& nonce) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      }));
}

TEST_F(PostDevicecheckTest, ServerError401) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_.Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      base::BindOnce([](mojom::Result result, const std::string& nonce) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      }));
}

TEST_F(PostDevicecheckTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
      .WillByDefault(Invoke(
          [](mojom::UrlRequestPtr request, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_.Request(
      "ff50981d-47de-4210-848d-995e186901a1",
      base::BindOnce([](mojom::Result result, const std::string& nonce) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(nonce, "");
      }));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
