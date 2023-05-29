/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/payment_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"

// npm run test -- brave_unit_tests --filter=PaymentUtilTest.*

namespace brave_rewards::internal::endpoint::payment {

class PaymentUtilTest : public MockLedgerTest {};

TEST_F(PaymentUtilTest, GetServerUrlDevelopment) {
  ledger().SetEnvironment(mojom::Environment::DEVELOPMENT);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://payment.rewards.brave.software/test");
}

TEST_F(PaymentUtilTest, GetServerUrlStaging) {
  ledger().SetEnvironment(mojom::Environment::STAGING);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://payment.rewards.bravesoftware.com/test");
}

TEST_F(PaymentUtilTest, GetServerUrlProduction) {
  ledger().SetEnvironment(mojom::Environment::PRODUCTION);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://payment.rewards.brave.com/test");
}

}  // namespace brave_rewards::internal::endpoint::payment
