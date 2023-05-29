/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/api/api_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"

// npm run test -- brave_unit_tests --filter=APIUtilTest.*

namespace brave_rewards::internal::endpoint::api {

class APIUtilTest : public MockLedgerTest {};

TEST_F(APIUtilTest, GetServerUrlDevelopment) {
  ledger().SetEnvironment(mojom::Environment::DEVELOPMENT);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://api.rewards.brave.software/test");
}

TEST_F(APIUtilTest, GetServerUrlStaging) {
  ledger().SetEnvironment(mojom::Environment::STAGING);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://api.rewards.bravesoftware.com/test");
}

TEST_F(APIUtilTest, GetServerUrlProduction) {
  ledger().SetEnvironment(mojom::Environment::PRODUCTION);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://api.rewards.brave.com/test");
}

}  // namespace brave_rewards::internal::endpoint::api
