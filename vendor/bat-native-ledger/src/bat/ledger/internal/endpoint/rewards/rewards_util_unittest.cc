/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoint/rewards/rewards_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsUtilTest.*

namespace ledger {
namespace endpoint {
namespace rewards {

class RewardsUtilTest : public testing::Test {
};

TEST(RewardsUtilTest, GetServerUrlDevelopment) {
  ledger::_environment = ledger::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl("/test");
  const std::string expected_url = "";
  ASSERT_EQ(url, "https://rewards-dev.brave.software/test");
}

TEST(RewardsUtilTest, GetServerUrlStaging) {
  ledger::_environment = ledger::Environment::STAGING;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://rewards-stg.bravesoftware.com/test");
}

TEST(RewardsUtilTest, GetServerUrlProduction) {
  ledger::_environment = ledger::Environment::PRODUCTION;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://rewards.brave.com/test");
}

}  // namespace rewards
}  // namespace endpoint
}  // namespace ledger
