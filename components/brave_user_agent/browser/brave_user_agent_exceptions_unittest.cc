/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_user_agent {

class BraveUserAgentExceptionsUnitTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  BraveUserAgentExceptions exceptions_ =
      BraveUserAgentExceptions::CreateForTesting();
};

TEST_F(BraveUserAgentExceptionsUnitTest, SingletonFollowsFeatureFlag) {
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndDisableFeature(features::kUseBraveUserAgent);
    EXPECT_EQ(nullptr, BraveUserAgentExceptions::GetInstance());
  }
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kUseBraveUserAgent);
  EXPECT_NE(nullptr, BraveUserAgentExceptions::GetInstance());
}

TEST_F(BraveUserAgentExceptionsUnitTest, TestCanShowBraveDomainsNotLoaded) {
  // Excepted domains not loaded; default to true.
  GURL url = GURL("https://brave.com");
  ASSERT_TRUE(exceptions_.CanShowBrave(url));
}

TEST_F(BraveUserAgentExceptionsUnitTest, TestCanShowBraveDomainsLoaded) {
  // Load excepted domains to hide brave
  const char* excepted_domains = R""""(
    brave.com
    site.example
    )"""";
  base::ScopedTempDir temp_dir_;
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath excepted_domains_path =
      temp_dir_.GetPath().AppendASCII("brave-checks.txt");
  ASSERT_TRUE(base::WriteFile(excepted_domains_path, excepted_domains));
  exceptions_.OnComponentReady(temp_dir_.GetPath());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !exceptions_.CanShowBrave(GURL("https://brave.com")); }));

  // Test excepted domains (hide we are Brave)
  ASSERT_FALSE(exceptions_.CanShowBrave(GURL("https://brave.com")));
  ASSERT_FALSE(exceptions_.CanShowBrave(GURL("https://brave.com/privacy")));
  ASSERT_FALSE(exceptions_.CanShowBrave(GURL("https://site.example")));
  // Test other domains (show we are Brave)
  ASSERT_TRUE(exceptions_.CanShowBrave(GURL("https://adifferentsite.example")));
  ASSERT_TRUE(exceptions_.CanShowBrave(GURL("https://youtube.com")));
  ASSERT_TRUE(exceptions_.CanShowBrave(GURL("https://github.io")));
  ASSERT_TRUE(exceptions_.CanShowBrave(GURL("https://github.com")));
}

}  // namespace brave_user_agent
