/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_utils.h"
#include "base/strings/strcat.h"
#include "bat/ledger/buildflags.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BitflyerUtilsTest.*

namespace ledger {
namespace endpoint {
namespace bitflyer {

class BitflyerUtilsTest : public testing::Test {};

TEST(BitflyerUtilsTest, GetServerUrlDevelopment) {
  ledger::_environment = mojom::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(BITFLYER_STAGING_URL), "/test"}));
}

TEST(BitflyerUtilsTest, GetServerUrlStaging) {
  ledger::_environment = mojom::Environment::STAGING;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, base::StrCat({BUILDFLAG(BITFLYER_STAGING_URL), "/test"}));
}

TEST(BitflyerUtilsTest, GetServerUrlProduction) {
  ledger::_environment = mojom::Environment::PRODUCTION;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://bitflyer.com/test");
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
