/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/utils.h"

#include "base/test/task_environment.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/ip_address.h"
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
  EXPECT_TRUE(IsUnstoppableDomainsTLD(GURL("http://test.888")));
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

TEST_F(UtilsUnitTest, Dot888Test) {
  net::IPAddress ip_address;
  // These tests were passing without .888 fix in `url_canon_ip.cc`.
  EXPECT_TRUE(GURL("http://1.1.888").is_valid());
  EXPECT_TRUE(ip_address.AssignFromIPLiteral("1.1.888"));
  EXPECT_TRUE(GURL("http://123.888").is_valid());
  EXPECT_TRUE(ip_address.AssignFromIPLiteral("123.888"));
  EXPECT_TRUE(GURL("http://1.123.888").is_valid());
  EXPECT_TRUE(ip_address.AssignFromIPLiteral("1.123.888"));
  EXPECT_TRUE(GURL("http://.com").is_valid());
  EXPECT_FALSE(ip_address.AssignFromIPLiteral(".com"));

  // These tests start passing with .888 fix in `url_canon_ip.cc`.
  const char* cases[] = {
      "test.888",         // Non-ipv4 component case.
      "test1.test2.888",  // Non-ipv4 component case.
      "1.2.3.4.888",      // Too many components case.
      "1.2.3.4.5.888",    // Too many components case.
      "555.888",          // Non-last component overflow case.
      "555.1.888",        // Non-last component overflow case.
      "555.1.1.888",      // Non-last component overflow case.
      "1.1.test.888",     // Some test
      "888.888",          // Some test
      "1.888.888",        // Some test
      ".888",             // Same as .com
  };

  for (auto* test_case : cases) {
    SCOPED_TRACE(testing::Message() << test_case);
    // Ok for being a host for url.
    EXPECT_TRUE(GURL(std::string("http://") + test_case).is_valid());
    // Still not ok to be an ipv4 address.
    EXPECT_FALSE(ip_address.AssignFromIPLiteral(test_case));
  }
}

}  // namespace decentralized_dns
