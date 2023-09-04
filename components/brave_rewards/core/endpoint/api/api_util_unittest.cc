/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/api/api_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=APIUtilTest.*

namespace brave_rewards::internal {
namespace endpoint {
namespace api {

class APIUtilTest : public testing::Test {};

TEST(APIUtilTest, GetServerUrlDevelopment) {
  _environment = mojom::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://api.rewards.brave.software/test");
}

TEST(APIUtilTest, GetServerUrlStaging) {
  _environment = mojom::Environment::STAGING;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://api.rewards.bravesoftware.com/test");
}

TEST(APIUtilTest, GetServerUrlProduction) {
  _environment = mojom::Environment::PRODUCTION;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://api.rewards.brave.com/test");
}

}  // namespace api
}  // namespace endpoint
}  // namespace brave_rewards::internal
