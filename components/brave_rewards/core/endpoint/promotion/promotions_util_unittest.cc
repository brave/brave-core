/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionsUtilTest.*

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class PromotionsUtilTest : public testing::Test {};

TEST(PromotionsUtilTest, GetServerUrlDevelopment) {
  _environment = mojom::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url,
            base::StrCat({BUILDFLAG(REWARDS_GRANT_DEV_ENDPOINT), "/test"}));
}

TEST(PromotionsUtilTest, GetServerUrlStaging) {
  _environment = mojom::Environment::STAGING;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url,
            base::StrCat({BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT), "/test"}));
}

TEST(PromotionsUtilTest, GetServerUrlProduction) {
  _environment = mojom::Environment::PRODUCTION;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url,
            base::StrCat({BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT), "/test"}));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
