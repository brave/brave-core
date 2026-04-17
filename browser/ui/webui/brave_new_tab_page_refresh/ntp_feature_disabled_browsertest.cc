/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

bool ElementExists(content::WebContents* web_contents,
                   std::string_view test_id) {
  constexpr std::string_view kScript = R"(
    Boolean(document.querySelector('[data-testid="' + $1 + '"]'))
  )";
  return content::EvalJs(web_contents, content::JsReplace(kScript, test_id))
      .ExtractBool();
}

void WaitForStableLayout(content::WebContents* web_contents) {
  constexpr std::string_view kScript = R"(
    new Promise((resolve) => {
      const stableAfter = 500
      const timeoutAfter = 4000
      let timeout = null

      const observer = new MutationObserver(onMutation)

      function done() {
        observer.disconnect()
        resolve()
      }

      function onMutation() {
        if (timeout) {
          clearTimeout(timeout)
        }
        timeout = setTimeout(done, stableAfter)
      }

      onMutation()

      observer.observe(document.documentElement, {
        childList: true,
        subtree: true,
      })

      setTimeout(done, timeoutAfter)
    })
  )";

  ASSERT_TRUE(
      content::EvalJs(web_contents, content::JsReplace(kScript)).is_ok());
}

}  // namespace

// Base class for tests that verify the NTP loads correctly when a feature is
// disabled via admin policy. Provides policy provider setup and DOM assertion
// helpers.
class NTPFeatureDisabledTestBase : public InProcessBrowserTest {
 protected:
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    EXPECT_CALL(policy_provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(
        &policy_provider_);
  }

  void SetPolicy(const std::string& key, base::Value value) {
    policy::PolicyMap policies;
    policies.Set(key, policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_PLATFORM, std::move(value), nullptr);
    policy_provider_.UpdateChromePolicy(policies);
  }

  void NavigateToNewTab() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                             GURL(chrome::kChromeUINewTabURL)));
    ASSERT_TRUE(content::WaitForLoadStop(GetActiveWebContents()));
  }

  void WaitForPageLayout() { WaitForStableLayout(GetActiveWebContents()); }

  void ExpectElementNotPresent(std::string_view test_id) {
    EXPECT_FALSE(ElementExists(GetActiveWebContents(), test_id))
        << "Element '" << test_id << "' should not be present";
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 private:
  testing::NiceMock<policy::MockConfigurationPolicyProvider> policy_provider_;
};

// --- Rewards disabled by policy ---

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
class NTPRewardsDisabledByPolicyTest : public NTPFeatureDisabledTestBase {
 protected:
  void SetUpInProcessBrowserTestFixture() override {
    NTPFeatureDisabledTestBase::SetUpInProcessBrowserTestFixture();
    SetPolicy(policy::key::kBraveRewardsDisabled, base::Value(true));
  }
};

IN_PROC_BROWSER_TEST_F(NTPRewardsDisabledByPolicyTest,
                       PageLoadsAndWidgetHidden) {
  NavigateToNewTab();
  WaitForPageLayout();
  ExpectElementNotPresent("rewards-widget-tab");
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

// --- VPN disabled by policy ---

#if BUILDFLAG(ENABLE_BRAVE_VPN)
class NTPVPNDisabledByPolicyTest : public NTPFeatureDisabledTestBase {
 protected:
  void SetUpInProcessBrowserTestFixture() override {
    NTPFeatureDisabledTestBase::SetUpInProcessBrowserTestFixture();
    SetPolicy(policy::key::kBraveVPNDisabled, base::Value(true));
  }
};

IN_PROC_BROWSER_TEST_F(NTPVPNDisabledByPolicyTest, PageLoadsAndWidgetHidden) {
  NavigateToNewTab();
  WaitForPageLayout();
  ExpectElementNotPresent("vpn-widget-tab");
}
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

// --- AI Chat disabled by policy ---

#if BUILDFLAG(ENABLE_AI_CHAT)
class NTPAIChatDisabledByPolicyTest : public NTPFeatureDisabledTestBase {
 protected:
  void SetUpInProcessBrowserTestFixture() override {
    NTPFeatureDisabledTestBase::SetUpInProcessBrowserTestFixture();
    SetPolicy(policy::key::kBraveAIChatEnabled, base::Value(false));
  }
};

IN_PROC_BROWSER_TEST_F(NTPAIChatDisabledByPolicyTest, PageLoadsAndChatHidden) {
  NavigateToNewTab();
  WaitForPageLayout();
  ExpectElementNotPresent("query-mode-toggle");
  ExpectElementNotPresent("ntp-chat-input");
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

// --- News disabled by policy ---

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
class NTPNewsDisabledByPolicyTest : public NTPFeatureDisabledTestBase {
 protected:
  void SetUpInProcessBrowserTestFixture() override {
    NTPFeatureDisabledTestBase::SetUpInProcessBrowserTestFixture();
    SetPolicy(policy::key::kBraveNewsDisabled, base::Value(true));
  }
};

IN_PROC_BROWSER_TEST_F(NTPNewsDisabledByPolicyTest, PageLoadsAndWidgetHidden) {
  NavigateToNewTab();
  WaitForPageLayout();
  ExpectElementNotPresent("news-widget-tab");
}
#endif  // BUILDFLAG(ENABLE_BRAVE_NEWS)
