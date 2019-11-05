/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/common/tor/pref_names.h"
#endif

using testing::_;
using testing::Return;
using NoTorPolicyBrowserTest = InProcessBrowserTest;

namespace policy {

class BravePolicyTest : public InProcessBrowserTest {
 protected:
  BravePolicyTest() {}
  ~BravePolicyTest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  MockConfigurationPolicyProvider provider_;
};

#if BUILDFLAG(ENABLE_TOR)
#if defined(OS_WIN)
// This policy only exists on Windows.
// Sets the tor policy before the browser is started.
class TorDisabledPolicyBrowserTest : public BravePolicyTest {
 public:
  TorDisabledPolicyBrowserTest() {}
  ~TorDisabledPolicyBrowserTest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    BravePolicyTest::SetUpInProcessBrowserTestFixture();

    PolicyMap policies;
    policies.Set(key::kTorDisabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_MACHINE, POLICY_SOURCE_PLATFORM,
                 std::make_unique<base::Value>(true), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
};

IN_PROC_BROWSER_TEST_F(TorDisabledPolicyBrowserTest, TorDisabledPrefValueTest) {
  EXPECT_TRUE(tor::TorProfileService::IsTorDisabled());
}

class TorEnabledPolicyBrowserTest : public BravePolicyTest {
 public:
  TorEnabledPolicyBrowserTest() {}
  ~TorEnabledPolicyBrowserTest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    BravePolicyTest::SetUpInProcessBrowserTestFixture();

    PolicyMap policies;
    policies.Set(key::kTorDisabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_MACHINE, POLICY_SOURCE_PLATFORM,
                 std::make_unique<base::Value>(false), nullptr);
    provider_.UpdateChromePolicy(policies);
  }
};

IN_PROC_BROWSER_TEST_F(TorEnabledPolicyBrowserTest, TorDisabledPrefValueTest) {
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabled());
}

// W/o TorDisabled group policy, kTorDisabled pref value should be false.
IN_PROC_BROWSER_TEST_F(NoTorPolicyBrowserTest,
                       DefaultTorDisabledPrefValueTest) {
  EXPECT_FALSE(tor::TorProfileService::IsTorDisabled());
}
#endif  // OS_WIN
#endif  // ENABLE_TOR

}  // namespace policy
