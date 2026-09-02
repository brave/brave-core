/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/common/brave_search_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_search/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

namespace brave_search {

class BraveSearchUtilsUnitTest : public testing::Test {
 protected:
  void SetUp() override {
#if BUILDFLAG(ENABLE_AI_CHAT)
    local_state_.registry()->RegisterBooleanPref(
        ai_chat::prefs::kNtpInputDayZeroEnabled, false);
#endif
  }

  TestingPrefServiceSimple local_state_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveSearchUtilsUnitTest, IsAllowedHost) {
  EXPECT_TRUE(IsAllowedHost(GURL("https://safesearch.brave.com")));
  EXPECT_TRUE(IsAllowedHost(GURL("https://safesearch.bravesoftware.com")));
  EXPECT_TRUE(IsAllowedHost(GURL("https://search-dev-local.brave.com")));
  EXPECT_TRUE(IsAllowedHost(GURL("https://search.brave.com")));
  EXPECT_TRUE(IsAllowedHost(GURL("https://search.brave.software")));
  EXPECT_TRUE(IsAllowedHost(GURL("https://search.bravesoftware.com")));

  // not https
  EXPECT_FALSE(IsAllowedHost(GURL("http://safesearch.brave.com")));
  // not allowed domains
  EXPECT_FALSE(IsAllowedHost(GURL("https://earch.brave.com")));
  EXPECT_FALSE(IsAllowedHost(GURL("https://brave.com")));
  EXPECT_FALSE(IsAllowedHost(GURL("https://a.search.brave.com")));
  EXPECT_FALSE(IsAllowedHost(GURL("https://search.brave.com.au")));
}

TEST_F(BraveSearchUtilsUnitTest, AppendsNewTabSource) {
  const GURL url("https://search.brave.com/search?q=test");
  EXPECT_EQ("https://search.brave.com/search?q=test&source=newtab",
            OverrideWithNewTabSource(url, &local_state_, false).spec());
}

TEST_F(BraveSearchUtilsUnitTest, AppendsNewTabV1Source) {
  feature_list_.InitAndEnableFeature(features::kSearchNewTabV1Source);
  const GURL url("https://search.brave.com/search?q=test");
  EXPECT_EQ("https://search.brave.com/search?q=test&source=newtab_v1",
            OverrideWithNewTabSource(url, &local_state_, false).spec());
}

#if BUILDFLAG(ENABLE_AI_CHAT)
TEST_F(BraveSearchUtilsUnitTest, AppendsNewTabV2Source) {
  feature_list_.InitWithFeatures(
      /*enabled_features=*/{ai_chat::features::kShowAIChatInputOnNewTabPage},
      /*disabled_features=*/{});
  const GURL url("https://search.brave.com/search?q=test");
  EXPECT_EQ("https://search.brave.com/search?q=test&source=newtab_v2",
            OverrideWithNewTabSource(url, &local_state_, false).spec());
}
#endif

}  // namespace brave_search
