/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_clobbered_claims/post_clobbered_claims.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PostClobberedClaimsTest.*

using ::testing::_;
using ::testing::Invoke;

namespace brave_rewards::core {
namespace endpoint {
namespace promotion {

class PostClobberedClaimsTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PostClobberedClaims> claims_;

  PostClobberedClaimsTest() {
    mock_ledger_client_ = std::make_unique<MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<MockLedgerImpl>(mock_ledger_client_.get());
    claims_ = std::make_unique<PostClobberedClaims>(mock_ledger_impl_.get());
  }
};

TEST_F(PostClobberedClaimsTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(std::move(corrupted_claims), [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_OK);
  });
}

TEST_F(PostClobberedClaimsTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(std::move(corrupted_claims), [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

TEST_F(PostClobberedClaimsTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(std::move(corrupted_claims), [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

TEST_F(PostClobberedClaimsTest, ServerErrorRandom) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 453;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  base::Value::List corrupted_claims;
  corrupted_claims.Append(base::Value("asfeq4gerg34gl3g34lg34g"));

  claims_->Request(std::move(corrupted_claims), [](const mojom::Result result) {
    EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::core
