/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "bat/ledger/internal/endpoint/payment/payment_util.h"
#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/option_keys.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PaymentUtilTest.*

namespace ledger {
namespace endpoint {
namespace payment {

class PaymentUtilTest : public testing::Test {
 public:
 protected:
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  base::test::TaskEnvironment task_environment_;

  PaymentUtilTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
  }

  ledger::MockLedgerImpl* mock_ledger() { return mock_ledger_impl_.get(); }

  void SetUp() override {
    ON_CALL(*mock_ledger_client_,
            GetStringOption(ledger::option::kPaymentServiceURL))
        .WillByDefault(testing::Return(""));
    EXPECT_CALL(*mock_ledger_client_,
                GetStringOption(ledger::option::kPaymentServiceURL))
        .Times(1);
  }
};

TEST_F(PaymentUtilTest, GetServerUrlDevelopment) {
  ledger::_environment = type::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl(mock_ledger(), "/test");
  task_environment_.RunUntilIdle();
  ASSERT_EQ(url, "https://payment.rewards.brave.software/test");
}

TEST_F(PaymentUtilTest, GetServerUrlStaging) {
  ledger::_environment = type::Environment::STAGING;
  const std::string url = GetServerUrl(mock_ledger(), "/test");
  task_environment_.RunUntilIdle();
  ASSERT_EQ(url, "https://payment.rewards.bravesoftware.com/test");
}

TEST_F(PaymentUtilTest, GetServerUrlProduction) {
  ledger::_environment = type::Environment::PRODUCTION;
  const std::string url = GetServerUrl(mock_ledger(), "/test");
  task_environment_.RunUntilIdle();
  ASSERT_EQ(url, "https://payment.rewards.brave.com/test");
}

TEST_F(PaymentUtilTest, GetServerUrlWithPaymentServiceURLSwitch) {
  const std::string test_payment_url = "https://test.payment";
  ON_CALL(*mock_ledger_client_,
          GetStringOption(ledger::option::kPaymentServiceURL))
      .WillByDefault(testing::Return(test_payment_url));

  ledger::_environment = type::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl(mock_ledger(), "/test");
  task_environment_.RunUntilIdle();
  ASSERT_EQ(url, test_payment_url + "/test");
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
