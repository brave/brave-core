/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/task/current_thread.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_user_agent {

class BraveUserAgentExceptionsUnitTest : public testing::Test {
 public:
  BraveUserAgentExceptionsUnitTest() {}

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(BraveUserAgentExceptionsUnitTest, TestCanShowBraveDomainsNotLoaded) {
  // BraveUserAgentExceptions returns nullptr when feature is disabled.
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(brave_user_agent::features::kUseBraveUserAgent);

  auto* brave_user_agent_exceptions = BraveUserAgentExceptions::GetInstance();
  // Excepted domains not loaded; default to true.
  GURL url = GURL("https://brave.com");
  ASSERT_TRUE(brave_user_agent_exceptions->CanShowBrave(url));
}

TEST_F(BraveUserAgentExceptionsUnitTest, TestCanShowBraveDomainsLoaded) {
  // BraveUserAgentExceptions returns nullptr when feature is disabled.
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(brave_user_agent::features::kUseBraveUserAgent);

  auto* brave_user_agent_exceptions = BraveUserAgentExceptions::GetInstance();

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
  brave_user_agent_exceptions->OnComponentReady(temp_dir_.GetPath());
  // run for callback to OnExceptedDomainsLoaded
  base::test::RunUntil(
      [&]() { return brave_user_agent_exceptions->is_ready_; });

  // Test excepted domains (hide we are Brave)
  ASSERT_FALSE(
      brave_user_agent_exceptions->CanShowBrave(GURL("https://brave.com")));
  ASSERT_FALSE(brave_user_agent_exceptions->CanShowBrave(
      GURL("https://brave.com/privacy")));
  ASSERT_FALSE(
      brave_user_agent_exceptions->CanShowBrave(GURL("https://site.example")));
  // Test other domains (show we are Brave)
  ASSERT_TRUE(brave_user_agent_exceptions->CanShowBrave(
      GURL("https://adifferentsite.example")));
  ASSERT_TRUE(
      brave_user_agent_exceptions->CanShowBrave(GURL("https://youtube.com")));
  ASSERT_TRUE(
      brave_user_agent_exceptions->CanShowBrave(GURL("https://github.io")));
  ASSERT_TRUE(
      brave_user_agent_exceptions->CanShowBrave(GURL("https://github.com")));
}

}  // namespace brave_user_agent
