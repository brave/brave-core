/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"

// npm run test -- brave_unit_tests --filter=PromotionsUtilTest.*

namespace brave_rewards::internal::endpoint::promotion {

class PromotionsUtilTest : public MockLedgerTest {};

TEST_F(PromotionsUtilTest, GetServerUrlDevelopment) {
  ledger().SetEnvironment(mojom::Environment::DEVELOPMENT);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url,
            base::StrCat({BUILDFLAG(REWARDS_GRANT_DEV_ENDPOINT), "/test"}));
}

TEST_F(PromotionsUtilTest, GetServerUrlStaging) {
  ledger().SetEnvironment(mojom::Environment::STAGING);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url,
            base::StrCat({BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT), "/test"}));
}

TEST_F(PromotionsUtilTest, GetServerUrlProduction) {
  ledger().SetEnvironment(mojom::Environment::PRODUCTION);
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url,
            base::StrCat({BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT), "/test"}));
}

}  // namespace brave_rewards::internal::endpoint::promotion
