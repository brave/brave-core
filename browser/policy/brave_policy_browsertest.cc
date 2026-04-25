/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_ids.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

using testing::_;
using testing::Return;
using NoTorPolicyBrowserTest = InProcessBrowserTest;

namespace policy {

class BravePolicyTest : public InProcessBrowserTest {
 protected:
  BravePolicyTest() = default;
  ~BravePolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  MockConfigurationPolicyProvider provider_;
};

#if BUILDFLAG(ENABLE_TOR)
// This policy only exists on Windows.
// Sets the tor policy before the browser is started.
class TorDisabledPolicyBrowserTest : public BravePolicyTest {
 public:
  TorDisabledPolicyBrowserTest() = default;
  ~TorDisabledPolicyBrowserTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    BravePolicyTest::SetUpInProcessBrowserTestFixture();

    PolicyMap policies;
    policies.Set(key::kTorDisabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_MACHINE, POLICY_SOURCE_PLATFORM,
                 base::Value(true), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
};

IN_PROC_BROWSER_TEST_F(TorDisabledPolicyBrowserTest, TorDisabledPrefValueTest) {
  // When policy is set, explicit setting doesn't change its pref value.
  TorProfileServiceFactory::SetTorDisabled(false);
  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
}

class TorEnabledPolicyBrowserTest : public BravePolicyTest {
 public:
  TorEnabledPolicyBrowserTest() = default;
  ~TorEnabledPolicyBrowserTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    BravePolicyTest::SetUpInProcessBrowserTestFixture();

    PolicyMap policies;
    policies.Set(key::kTorDisabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_MACHINE, POLICY_SOURCE_PLATFORM,
                 base::Value(false), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
};

IN_PROC_BROWSER_TEST_F(TorEnabledPolicyBrowserTest, TorDisabledPrefValueTest) {
  // When policy is set, explicit setting doesn't change its pref value.
  TorProfileServiceFactory::SetTorDisabled(true);
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
}

// Test that Tor settings are hidden in chrome://settings when disabled by
// policy
IN_PROC_BROWSER_TEST_F(TorDisabledPolicyBrowserTest,
                       TorSettingsHiddenByPolicy) {
  // Verify Tor is disabled by policy
  ASSERT_TRUE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  ASSERT_TRUE(TorProfileServiceFactory::IsTorManaged(browser()->profile()));

  // Navigate to settings page
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings/")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Check that braveTorDisabledByPolicy is true when disabled by policy
  std::string script = R"(
    loadTimeData.getBoolean('braveTorDisabledByPolicy');
  )";

  EXPECT_EQ(true, content::EvalJs(web_contents, script));
}

// Test that Tor settings are visible when enabled by policy
IN_PROC_BROWSER_TEST_F(TorEnabledPolicyBrowserTest,
                       TorSettingsVisibleWhenEnabledByPolicy) {
  // Verify Tor is enabled (not disabled) by policy
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled(browser()->profile()));
  EXPECT_TRUE(TorProfileServiceFactory::IsTorManaged(browser()->profile()));

  // Navigate to settings page
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings/")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Check that braveTorDisabledByPolicy is false when enabled by policy
  std::string script = R"(
    loadTimeData.getBoolean('braveTorDisabledByPolicy');
  )";

  EXPECT_EQ(false, content::EvalJs(web_contents, script));
}
#endif

template <bool enable>
class BrowserAddPersonPolicyTest : public BravePolicyTest {
 public:
  BrowserAddPersonPolicyTest() = default;
  ~BrowserAddPersonPolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    BravePolicyTest::SetUpInProcessBrowserTestFixture();

    PolicyMap policies;
    policies.Set(key::kBrowserAddPersonEnabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_MACHINE, POLICY_SOURCE_PLATFORM,
                 base::Value(enable), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
};

using BrowserAddPersonEnabledPolicyTest = BrowserAddPersonPolicyTest<true>;
using BrowserAddPersonDisabledPolicyTest = BrowserAddPersonPolicyTest<false>;

IN_PROC_BROWSER_TEST_F(BrowserAddPersonEnabledPolicyTest,
                       AddNewProfileEnabled) {
  auto* command_controller = browser()->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
}

IN_PROC_BROWSER_TEST_F(BrowserAddPersonDisabledPolicyTest,
                       AddNewProfileDisabled) {
  auto* command_controller = browser()->command_controller();
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_ADD_NEW_PROFILE));
}

template <bool enable>
class BrowserGuestModePolicyTest : public BravePolicyTest {
 public:
  BrowserGuestModePolicyTest() = default;
  ~BrowserGuestModePolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    BravePolicyTest::SetUpInProcessBrowserTestFixture();

    PolicyMap policies;
    policies.Set(key::kBrowserGuestModeEnabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_MACHINE, POLICY_SOURCE_PLATFORM,
                 base::Value(enable), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
};

using BrowserGuestModeEnabledPolicyTest = BrowserGuestModePolicyTest<true>;
using BrowserGuestModeDisabledPolicyTest = BrowserGuestModePolicyTest<false>;

IN_PROC_BROWSER_TEST_F(BrowserGuestModeEnabledPolicyTest,
                       OpenGuestProfileEnabled) {
  auto* command_controller = browser()->command_controller();
  EXPECT_TRUE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
}

IN_PROC_BROWSER_TEST_F(BrowserGuestModeDisabledPolicyTest,
                       OpenGuestProfileDisabled) {
  auto* command_controller = browser()->command_controller();
  EXPECT_FALSE(command_controller->IsCommandEnabled(IDC_OPEN_GUEST_PROFILE));
}

}  // namespace policy
