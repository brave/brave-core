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
  EXPECT_TRUE(IsUnstoppableDomainsTLD("test.crypto"));
  EXPECT_FALSE(IsUnstoppableDomainsTLD("test.com"));
  EXPECT_FALSE(IsUnstoppableDomainsTLD("test.eth"));
  EXPECT_FALSE(IsUnstoppableDomainsTLD("crypto"));
}

TEST_F(UtilsUnitTest, IsUnstoppableDomainsResolveMethodAsk) {
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodAsk(local_state()));

  SetUnstoppableDomainsResolveMethod(local_state(),
                                     ResolveMethodTypes::ENABLED);
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodAsk(local_state()));
}

TEST_F(UtilsUnitTest, IsUnstoppableDomainsResolveMethodEnabled) {
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEnabled(local_state()));

  SetUnstoppableDomainsResolveMethod(local_state(),
                                     ResolveMethodTypes::ENABLED);
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodEnabled(local_state()));
}

TEST_F(UtilsUnitTest, IsENSTLD) {
  EXPECT_TRUE(IsENSTLD("test.eth"));
  EXPECT_FALSE(IsENSTLD("test.com"));
  EXPECT_FALSE(IsENSTLD("test.crypto"));
  EXPECT_FALSE(IsENSTLD("eth"));
}

TEST_F(UtilsUnitTest, IsSnsTLD) {
  EXPECT_TRUE(IsSnsTLD("test.sol"));
  EXPECT_FALSE(IsSnsTLD("test.com"));
  EXPECT_FALSE(IsSnsTLD("test.crypto"));
  EXPECT_FALSE(IsSnsTLD("eth"));
}

TEST_F(UtilsUnitTest, IsENSResolveMethodAsk) {
  EXPECT_TRUE(IsENSResolveMethodAsk(local_state()));

  SetENSResolveMethod(local_state(), ResolveMethodTypes::ENABLED);
  EXPECT_FALSE(IsENSResolveMethodAsk(local_state()));
}

TEST_F(UtilsUnitTest, IsENSResolveMethodEnabledd) {
  EXPECT_FALSE(IsENSResolveMethodEnabled(local_state()));

  SetENSResolveMethod(local_state(), ResolveMethodTypes::ENABLED);
  EXPECT_TRUE(IsENSResolveMethodEnabled(local_state()));
}

TEST_F(UtilsUnitTest, IsSnsResolveMethodAsk) {
  EXPECT_TRUE(IsSnsResolveMethodAsk(local_state()));

  SetSnsResolveMethod(local_state(), ResolveMethodTypes::ENABLED);
  EXPECT_FALSE(IsSnsResolveMethodAsk(local_state()));
}

TEST_F(UtilsUnitTest, IsSnsResolveMethodEnabledd) {
  EXPECT_FALSE(IsSnsResolveMethodEnabled(local_state()));

  SetSnsResolveMethod(local_state(), ResolveMethodTypes::ENABLED);
  EXPECT_TRUE(IsSnsResolveMethodEnabled(local_state()));
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
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEnabled(local_state()));
  EXPECT_FALSE(IsENSResolveMethodEnabled(local_state()));

  MigrateObsoleteLocalStatePrefs(local_state());
  EXPECT_FALSE(local_state()->HasPrefPath(kUnstoppableDomainsResolveMethod));
  EXPECT_FALSE(local_state()->HasPrefPath(kENSResolveMethod));
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodAsk(local_state()));
  EXPECT_TRUE(IsENSResolveMethodAsk(local_state()));
}

TEST_F(UtilsUnitTest, SNSResolveMethodMigration) {
  // Ask
  EXPECT_TRUE(IsSnsResolveMethodAsk(local_state()));
  EXPECT_FALSE(local_state()->GetBoolean(kSnsResolveMethodMigrated));

  MigrateObsoleteLocalStatePrefs(local_state());

  EXPECT_TRUE(IsSnsResolveMethodAsk(local_state()));
  EXPECT_TRUE(local_state()->GetBoolean(kSnsResolveMethodMigrated));

  // Enabled
  local_state()->SetBoolean(kSnsResolveMethodMigrated, false);
  local_state()->SetInteger(kSnsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));
  EXPECT_TRUE(IsSnsResolveMethodEnabled(local_state()));
  EXPECT_FALSE(local_state()->GetBoolean(kSnsResolveMethodMigrated));

  MigrateObsoleteLocalStatePrefs(local_state());

  EXPECT_TRUE(IsSnsResolveMethodAsk(local_state()));
  EXPECT_TRUE(local_state()->GetBoolean(kSnsResolveMethodMigrated));

  // Disabled
  local_state()->SetBoolean(kSnsResolveMethodMigrated, false);
  local_state()->SetInteger(kSnsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::DISABLED));
  EXPECT_FALSE(local_state()->GetBoolean(kSnsResolveMethodMigrated));

  MigrateObsoleteLocalStatePrefs(local_state());

  EXPECT_EQ(local_state()->GetInteger(kSnsResolveMethod),
            static_cast<int>(ResolveMethodTypes::DISABLED));
  EXPECT_TRUE(local_state()->GetBoolean(kSnsResolveMethodMigrated));
}

}  // namespace decentralized_dns
