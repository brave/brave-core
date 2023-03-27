/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/put_devicecheck/put_devicecheck.h"

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

// npm run test -- brave_unit_tests --filter=PutDevicecheckTest.*

using ::testing::_;
using ::testing::Invoke;

namespace brave_rewards::core {
namespace endpoint {
namespace promotion {

class PutDevicecheckTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<PutDevicecheck> devicecheck_;

  PutDevicecheckTest() {
    mock_ledger_client_ = std::make_unique<MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<MockLedgerImpl>(mock_ledger_client_.get());
    devicecheck_ = std::make_unique<PutDevicecheck>(mock_ledger_impl_.get());
  }
};

TEST_F(PutDevicecheckTest, ServerOK) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 200;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_->Request("dsfqwf4f901a1", "asdfasdf", "fsadfasdfff4901a1",
                        base::BindOnce([](mojom::Result result) {
                          EXPECT_EQ(result, mojom::Result::LEDGER_OK);
                        }));
}

TEST_F(PutDevicecheckTest, ServerError400) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 400;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_->Request("dsfqwf4f901a1", "asdfasdf", "fsadfasdfff4901a1",
                        base::BindOnce([](mojom::Result result) {
                          EXPECT_EQ(result, mojom::Result::CAPTCHA_FAILED);
                        }));
}

TEST_F(PutDevicecheckTest, ServerError401) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 401;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_->Request("dsfqwf4f901a1", "asdfasdf", "fsadfasdfff4901a1",
                        base::BindOnce([](mojom::Result result) {
                          EXPECT_EQ(result, mojom::Result::CAPTCHA_FAILED);
                        }));
}

TEST_F(PutDevicecheckTest, ServerError500) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr request, LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = 500;
            response.url = request->url;
            response.body = "";
            std::move(callback).Run(response);
          }));

  devicecheck_->Request("dsfqwf4f901a1", "asdfasdf", "fsadfasdfff4901a1",
                        base::BindOnce([](mojom::Result result) {
                          EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
                        }));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::core
