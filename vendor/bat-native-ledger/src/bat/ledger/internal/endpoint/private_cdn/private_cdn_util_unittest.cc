/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoint/private_cdn/private_cdn_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PrivateCDNTest.*

namespace ledger {
namespace endpoint {
namespace private_cdn {

class PrivateCDNTest : public testing::Test {
};

TEST(PrivateCDNTest, GetServerUrlDevelopment) {
  ledger::_environment = ledger::Environment::DEVELOPMENT;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://pcdn.brave.software/test");
}

TEST(PrivateCDNTest, GetServerUrlStaging) {
  ledger::_environment = ledger::Environment::STAGING;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://pcdn.bravesoftware.com/test");
}

TEST(PrivateCDNTest, GetServerUrlProduction) {
  ledger::_environment = ledger::Environment::PRODUCTION;
  const std::string url = GetServerUrl("/test");
  ASSERT_EQ(url, "https://pcdn.brave.com/test");
}

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger
