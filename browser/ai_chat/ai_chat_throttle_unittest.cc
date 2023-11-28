/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/content/browser/ai_chat_throttle.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {
constexpr char kTestProfileName[] = "TestProfile";
}  // namespace

class AiChatThrottleUnitTest : public testing::Test {
 public:
  AiChatThrottleUnitTest() = default;
  AiChatThrottleUnitTest(const AiChatThrottleUnitTest&) = delete;
  AiChatThrottleUnitTest& operator=(const AiChatThrottleUnitTest&) = delete;
  ~AiChatThrottleUnitTest() override = default;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    Profile* profile = profile_manager_->CreateTestingProfile(kTestProfileName);

    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile, nullptr);

    features_.InitAndEnableFeature(ai_chat::features::kAIChat);
  }

  void TearDown() override {
    web_contents_.reset();
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  base::test::ScopedFeatureList features_;
};

TEST_F(AiChatThrottleUnitTest, CancelNavigationFromTab) {
  content::MockNavigationHandle test_handle(web_contents());

  test_handle.set_url(GURL("chrome-untrusted://chat"));

#if BUILDFLAG(IS_ANDROID)
  ui::PageTransition transition = ui::PageTransitionFromInt(
      ui::PageTransition::PAGE_TRANSITION_FROM_ADDRESS_BAR);
#else
  ui::PageTransition transition = ui::PageTransitionFromInt(
      ui::PageTransition::PAGE_TRANSITION_FROM_ADDRESS_BAR |
      ui::PageTransition::PAGE_TRANSITION_TYPED);
#endif

  test_handle.set_page_transition(transition);

  std::unique_ptr<AiChatThrottle> throttle =
      AiChatThrottle::MaybeCreateThrottleFor(&test_handle);
  EXPECT_NE(throttle.get(), nullptr);

  EXPECT_EQ(content::NavigationThrottle::CANCEL_AND_IGNORE,
            throttle->WillStartRequest().action());
}

TEST_F(AiChatThrottleUnitTest, AllowNavigationFromPanel) {
  content::MockNavigationHandle test_handle(web_contents());

  test_handle.set_url(GURL("chrome-untrusted://chat"));

#if BUILDFLAG(IS_ANDROID)
  ui::PageTransition transition =
      ui::PageTransitionFromInt(ui::PageTransition::PAGE_TRANSITION_FROM_API);
#else
  ui::PageTransition transition = ui::PageTransitionFromInt(
      ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL);
#endif

  test_handle.set_page_transition(transition);

  std::unique_ptr<AiChatThrottle> throttle =
      AiChatThrottle::MaybeCreateThrottleFor(&test_handle);
  EXPECT_EQ(throttle.get(), nullptr);
}

}  // namespace ai_chat
