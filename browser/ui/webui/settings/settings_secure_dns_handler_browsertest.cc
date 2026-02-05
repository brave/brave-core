/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This is a minimal version of the upstream browser test:
// //chrome/browser/ui/webui/settings/settings_secure_dns_handler_browsertest.cc
//
// Only the `OtherPoliciesSet` test deviates from Chromium. For more info, see:
// https://github.com/brave/brave-browser/issues/46011
//
// NOTE: there is a filter added to exclude the upstream version of this test

#include "chrome/browser/ui/webui/settings/settings_secure_dns_handler.h"

#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/win_util.h"
#endif

namespace settings {

class BraveTestSecureDnsHandler : public SecureDnsHandler {
 public:
  // Pull WebUIMessageHandler::set_web_ui() into public so tests can call it.
  using SecureDnsHandler::set_web_ui;
};

class BraveSecureDnsHandlerTest : public InProcessBrowserTest {
 public:
  BraveSecureDnsHandlerTest(const BraveSecureDnsHandlerTest&) = delete;
  BraveSecureDnsHandlerTest& operator=(const BraveSecureDnsHandlerTest&) =
      delete;

 protected:
#if BUILDFLAG(IS_WIN)
  BraveSecureDnsHandlerTest()
      // Mark as not enterprise managed to prevent the secure DNS mode from
      // being downgraded to off.
      : scoped_domain_(false) {}
#else
  BraveSecureDnsHandlerTest() = default;
#endif
  ~BraveSecureDnsHandlerTest() override = default;

  // InProcessBrowserTest:
  void SetUpInProcessBrowserTestFixture() override {
    // Initialize user policy.
    provider_.SetDefaultReturns(/*is_initialization_complete_return=*/true,
                                /*is_first_policy_load_complete_return=*/true);
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  void SetUpOnMainThread() override {
    handler_ = std::make_unique<BraveTestSecureDnsHandler>();
    web_ui_.set_web_contents(
        browser()->tab_strip_model()->GetActiveWebContents());
    handler_->set_web_ui(&web_ui_);
    handler_->RegisterMessages();
    handler_->AllowJavascriptForTesting();
    base::RunLoop().RunUntilIdle();
  }

  void TearDownOnMainThread() override { handler_.reset(); }

  // Updates out-params from the last message sent to WebUI about a secure DNS
  // change. Returns false if the message was invalid or not found.
  bool GetLastSettingsChangedMessage(std::string* out_secure_dns_mode,
                                     std::string* out_doh_config,
                                     int* out_management_mode) {
    for (const std::unique_ptr<content::TestWebUI::CallData>& data :
         base::Reversed(web_ui_.call_data())) {
      if (data->function_name() != "cr.webUIListenerCallback" ||
          !data->arg1()->is_string() ||
          data->arg1()->GetString() != "secure-dns-setting-changed") {
        continue;
      }

      const base::DictValue* dict = data->arg2()->GetIfDict();
      if (!dict) {
        return false;
      }

      // Get the secure DNS mode.
      const std::string* secure_dns_mode = dict->FindString("mode");
      if (!secure_dns_mode) {
        return false;
      }
      *out_secure_dns_mode = *secure_dns_mode;

      // Get the DoH config string.
      const std::string* doh_config = dict->FindString("config");
      if (!doh_config) {
        return false;
      }
      *out_doh_config = *doh_config;

      // Get the forced management description.
      std::optional<int> management_mode = dict->FindInt("managementMode");
      if (!management_mode.has_value()) {
        return false;
      }
      *out_management_mode = *management_mode;

      return true;
    }
    return false;
  }

  // Sets a policy update which will cause power pref managed change.
  void SetPolicyForPolicyKey(policy::PolicyMap* policy_map,
                             const std::string& policy_key,
                             base::Value value) {
    policy_map->Set(policy_key, policy::POLICY_LEVEL_MANDATORY,
                    policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                    std::move(value), nullptr);
    provider_.UpdateChromePolicy(*policy_map);
    base::RunLoop().RunUntilIdle();
  }

  std::unique_ptr<BraveTestSecureDnsHandler> handler_;
  content::TestWebUI web_ui_;
  testing::NiceMock<policy::MockConfigurationPolicyProvider> provider_;

 private:
#if BUILDFLAG(IS_WIN)
  base::win::ScopedDomainStateForTesting scoped_domain_;
#endif
};

// If an install is considered managed (one or more policies in place),
// Chromium will disable the secure DNS feature (set to `off`). This was
// intentionally done upstream to let the administrator control the policy.
// Brave removes this restriction. Managed or not, the value can be modified.
IN_PROC_BROWSER_TEST_F(BraveSecureDnsHandlerTest, OtherPoliciesSet) {
  policy::PolicyMap policy_map;
  SetPolicyForPolicyKey(&policy_map, policy::key::kIncognitoModeAvailability,
                        base::Value(1));

  PrefService* local_state = g_browser_process->local_state();
  local_state->SetString(prefs::kDnsOverHttpsMode,
                         SecureDnsConfig::kModeSecure);

  std::string secure_dns_mode;
  std::string doh_config;
  int management_mode;
  EXPECT_TRUE(GetLastSettingsChangedMessage(&secure_dns_mode, &doh_config,
                                            &management_mode));

  // This directly tests the override.
  EXPECT_FALSE(SystemNetworkContextManager::GetStubResolverConfigReader()
                   ->ShouldDisableDohForManaged());

  // Indirectly test the override.
  EXPECT_EQ(SecureDnsConfig::kModeSecure, secure_dns_mode);
  EXPECT_EQ(static_cast<int>(SecureDnsConfig::ManagementMode::kNoOverride),
            management_mode);
}

}  // namespace settings
