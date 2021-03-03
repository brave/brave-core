/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/utils.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/features.h"
#include "brave/components/unstoppable_domains/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace unstoppable_domains {

class UtilsUnitTest : public testing::TestWithParam<bool> {
 public:
  UtilsUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()),
        feature_enabled_(GetParam()) {}
  ~UtilsUnitTest() override = default;

  void SetUp() override {
    if (feature_enabled_) {
      feature_list_.InitAndEnableFeature(features::kUnstoppableDomains);
    } else {
      feature_list_.InitAndDisableFeature(features::kUnstoppableDomains);
    }
  }

  PrefService* local_state() { return local_state_.Get(); }
  bool feature_enabled() { return feature_enabled_; }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  ScopedTestingLocalState local_state_;
  bool feature_enabled_;
};

TEST_P(UtilsUnitTest, IsUnstoppableDomainsTLD) {
  EXPECT_TRUE(IsUnstoppableDomainsTLD(GURL("http://test.crypto")));
  EXPECT_FALSE(IsUnstoppableDomainsTLD(GURL("http://test.com")));
  EXPECT_FALSE(IsUnstoppableDomainsTLD(GURL("http://crypto")));
}

TEST_P(UtilsUnitTest, IsUnstoppableDomainsEnabled) {
  EXPECT_EQ(feature_enabled(), IsUnstoppableDomainsEnabled());
}

TEST_P(UtilsUnitTest, IsResolveMethodAsk) {
  EXPECT_EQ(feature_enabled(), IsResolveMethodAsk(local_state()));

  local_state()->SetInteger(
      kResolveMethod, static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  EXPECT_FALSE(IsResolveMethodAsk(local_state()));
}

TEST_P(UtilsUnitTest, IsResolveMethodDoH) {
  EXPECT_FALSE(IsResolveMethodDoH(local_state()));

  local_state()->SetInteger(
      kResolveMethod, static_cast<int>(ResolveMethodTypes::DNS_OVER_HTTPS));
  EXPECT_EQ(feature_enabled(), IsResolveMethodDoH(local_state()));
}

INSTANTIATE_TEST_SUITE_P(/* no prefix */, UtilsUnitTest, testing::Bool());

}  // namespace unstoppable_domains
