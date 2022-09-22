/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/core/utils.h"

#include "base/test/task_environment.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace decentralized_dns {

class UtilsUnitTest : public testing::Test {
 public:
  UtilsUnitTest() : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~UtilsUnitTest() override = default;

  PrefService* local_state() { return local_state_.Get(); }

 private:
  base::test::TaskEnvironment task_environment_;
  ScopedTestingLocalState local_state_;
};

TEST_F(UtilsUnitTest, IsUnstoppableDomainsTLD) {
  EXPECT_TRUE(IsUnstoppableDomainsTLD(GURL("http://test.crypto")));
  EXPECT_FALSE(IsUnstoppableDomainsTLD(GURL("http://test.com")));
  EXPECT_FALSE(IsUnstoppableDomainsTLD(GURL("http://test.eth")));
  EXPECT_FALSE(IsUnstoppableDomainsTLD(GURL("http://crypto")));
}

TEST_F(UtilsUnitTest, IsUnstoppableDomainsResolveMethodAsk) {
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodAsk(local_state()));

  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodAsk(local_state()));
}

TEST_F(UtilsUnitTest, IsUnstoppableDomainsResolveMethodEthereum) {
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));

  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));
}

TEST_F(UtilsUnitTest, IsENSTLD) {
  EXPECT_TRUE(IsENSTLD(GURL("http://test.eth")));
  EXPECT_FALSE(IsENSTLD(GURL("http://test.com")));
  EXPECT_FALSE(IsENSTLD(GURL("http://test.crypto")));
  EXPECT_FALSE(IsENSTLD(GURL("http://eth")));
}

TEST_F(UtilsUnitTest, IsENSResolveMethodAsk) {
  EXPECT_TRUE(IsENSResolveMethodAsk(local_state()));

  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_FALSE(IsENSResolveMethodAsk(local_state()));
}

TEST_F(UtilsUnitTest, IsENSResolveMethodEthereum) {
  EXPECT_FALSE(IsENSResolveMethodEthereum(local_state()));

  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(IsENSResolveMethodEthereum(local_state()));
}

TEST_F(UtilsUnitTest, ResolveMethodMigration) {
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodAsk(local_state()));
  EXPECT_TRUE(IsENSResolveMethodAsk(local_state()));

  local_state()->SetInteger(
      kUnstoppableDomainsResolveMethod,
      static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS));
  local_state()->SetInteger(
      kENSResolveMethod,
      static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS));
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodAsk(local_state()));
  EXPECT_FALSE(IsENSResolveMethodAsk(local_state()));
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));
  EXPECT_FALSE(IsENSResolveMethodEthereum(local_state()));

  MigrateObsoleteLocalStatePrefs(local_state());
  EXPECT_FALSE(local_state()->HasPrefPath(kUnstoppableDomainsResolveMethod));
  EXPECT_FALSE(local_state()->HasPrefPath(kENSResolveMethod));
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodAsk(local_state()));
  EXPECT_TRUE(IsENSResolveMethodAsk(local_state()));
}

}  // namespace decentralized_dns
