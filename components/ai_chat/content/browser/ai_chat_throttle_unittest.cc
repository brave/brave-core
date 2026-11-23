/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_throttle.h"

#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ai_chat {

class AIChatThrottleUnitTest : public content::RenderViewHostTestHarness,
                               public ::testing::WithParamInterface<bool> {
 public:
  AIChatThrottleUnitTest() = default;
  AIChatThrottleUnitTest(const AIChatThrottleUnitTest&) = delete;
  AIChatThrottleUnitTest& operator=(const AIChatThrottleUnitTest&) = delete;
  ~AIChatThrottleUnitTest() override = default;

  void SetUp() override {
    features_.InitWithFeatureStates({
        {ai_chat::features::kAIChat, true},
        {ai_chat::features::kAIChatHistory, IsAIChatHistoryEnabled()},
    });
    content::RenderViewHostTestHarness::SetUp();
    prefs_.registry()->RegisterBooleanPref(ai_chat::prefs::kEnabledByPolicy,
                                           true);
    user_prefs::UserPrefs::Set(browser_context(), &prefs_);
  }

  bool IsAIChatHistoryEnabled() { return GetParam(); }

 private:
  base::test::ScopedFeatureList features_;
  // Outlives the browser context, which the harness destroys in TearDown().
  TestingPrefServiceSimple prefs_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatThrottleUnitTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<AIChatThrottleUnitTest::ParamType>& info) {
      return base::StrCat({"History", info.param ? "Enabled" : "Disabled"});
    });

TEST_P(AIChatThrottleUnitTest, CancelNavigationFromTab) {
  content::MockNavigationHandle test_handle(web_contents());

  test_handle.set_url(GURL(kAIChatUIURL));

  ui::PageTransition transition = ui::PageTransitionFromInt(
      ui::PageTransition::PAGE_TRANSITION_FROM_ADDRESS_BAR |
      ui::PageTransition::PAGE_TRANSITION_TYPED);

  test_handle.set_page_transition(transition);
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);

  AIChatThrottle::MaybeCreateAndAdd(registry);

#if !BUILDFLAG(IS_ANDROID)
  if (IsAIChatHistoryEnabled()) {
    EXPECT_TRUE(registry.throttles().empty());
  } else {
    ASSERT_FALSE(registry.throttles().empty());
    EXPECT_EQ(content::NavigationThrottle::CANCEL_AND_IGNORE,
              registry.throttles().back()->WillStartRequest().action());
  }
#else
  EXPECT_TRUE(registry.throttles().empty());
#endif
}

TEST_P(AIChatThrottleUnitTest, CancelNavigationToFrame) {
  content::MockNavigationHandle test_handle(web_contents());

  test_handle.set_url(GURL(kAIChatUntrustedConversationUIURL));

  ui::PageTransition transition = ui::PageTransitionFromInt(
      ui::PageTransition::PAGE_TRANSITION_FROM_ADDRESS_BAR |
      ui::PageTransition::PAGE_TRANSITION_TYPED);

  test_handle.set_page_transition(transition);
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);

  AIChatThrottle::MaybeCreateAndAdd(registry);
#if !BUILDFLAG(IS_ANDROID)
  ASSERT_FALSE(registry.throttles().empty());
  EXPECT_EQ(content::NavigationThrottle::CANCEL_AND_IGNORE,
            registry.throttles().back()->WillStartRequest().action());
#else
  EXPECT_TRUE(registry.throttles().empty());
#endif
}

TEST_P(AIChatThrottleUnitTest, AllowNavigationFromPanel) {
  content::MockNavigationHandle test_handle(web_contents());

  test_handle.set_url(GURL(kAIChatUIURL));

#if BUILDFLAG(IS_ANDROID)
  ui::PageTransition transition =
      ui::PageTransitionFromInt(ui::PageTransition::PAGE_TRANSITION_FROM_API);
#else
  ui::PageTransition transition = ui::PageTransitionFromInt(
      ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL);
#endif

  test_handle.set_page_transition(transition);
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);

  AIChatThrottle::MaybeCreateAndAdd(registry);
  EXPECT_TRUE(registry.throttles().empty());
}

}  // namespace ai_chat
