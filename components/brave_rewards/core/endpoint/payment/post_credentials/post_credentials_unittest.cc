/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/payment/post_credentials/post_credentials.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostCredentialsTest.*

using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace payment {

class PostCredentialsTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  PostCredentials creds_{&mock_ledger_impl_};
};

TEST_F(PostCredentialsTest, ServerOK) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), base::BindOnce([](mojom::Result result) {
                   EXPECT_EQ(result, mojom::Result::LEDGER_OK);
                 }));
}

TEST_F(PostCredentialsTest, ServerError400) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), base::BindOnce([](mojom::Result result) {
                   EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                 }));
}

TEST_F(PostCredentialsTest, ServerError409) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 409;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), base::BindOnce([](mojom::Result result) {
                   EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                 }));
}

TEST_F(PostCredentialsTest, ServerError500) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), base::BindOnce([](mojom::Result result) {
                   EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                 }));
}

TEST_F(PostCredentialsTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_impl_.rewards_service(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List blinded;
  blinded.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  creds_.Request("pl2okf23-f2f02kf2fm2-msdkfsodkfds",
                 "ff50981d-47de-4210-848d-995e186901a1", "single-use",
                 std::move(blinded), base::BindOnce([](mojom::Result result) {
                   EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                 }));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
