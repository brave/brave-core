/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/payment_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PaymentUtilTest.*

namespace ledger {
namespace endpoint {
namespace payment {

class PaymentUtilTest : public testing::Test {};

TEST(PaymentUtilTest, GetServerUrlDevelopment) {
  ledger::_environment = mojom::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl("/test");
  const std::string expected_url = "";
  ASSERT_EQ(url, "https://payment.rewards.brave.software/test");
}

TEST(PaymentUtilTest, GetServerUrlStaging) {
  ledger::_environment = mojom::Environment::STAGING;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://payment.rewards.bravesoftware.com/test");
}

TEST(PaymentUtilTest, GetServerUrlProduction) {
  ledger::_environment = mojom::Environment::PRODUCTION;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://payment.rewards.brave.com/test");
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
