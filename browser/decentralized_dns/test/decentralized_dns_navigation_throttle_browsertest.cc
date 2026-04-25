/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/ens_resolver_task.h"
#include "brave/components/decentralized_dns/content/decentralized_dns_opt_in_page.h"
#include "brave/components/decentralized_dns/content/ens_offchain_lookup_opt_in_page.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using brave_wallet::EnsResolverTask;
using brave_wallet::EnsResolverTaskError;
using brave_wallet::EnsResolverTaskResult;

namespace {

security_interstitials::SecurityInterstitialPage* GetCurrentInterstitial(
    content::WebContents* web_contents) {
  security_interstitials::SecurityInterstitialTabHelper* helper =
      security_interstitials::SecurityInterstitialTabHelper::FromWebContents(
          web_contents);
  if (!helper) {
    return nullptr;
  }
  return helper->GetBlockingPageForCurrentlyCommittedNavigationForTesting();
}

security_interstitials::SecurityInterstitialPage::TypeID GetInterstitialType(
    content::WebContents* web_contents) {
  security_interstitials::SecurityInterstitialPage* page =
      GetCurrentInterstitial(web_contents);
  if (!page) {
    return nullptr;
  }
  return page->GetTypeForTesting();
}

void SendInterstitialCommand(
    content::WebContents* web_contents,
    security_interstitials::SecurityInterstitialCommand command) {
  GetCurrentInterstitial(web_contents)
      ->CommandReceived(base::NumberToString(command));
}

void SendInterstitialCommandSync(
    Browser* browser,
    security_interstitials::SecurityInterstitialCommand command) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  content::TestNavigationObserver navigation_observer(web_contents, 1);
  SendInterstitialCommand(web_contents, command);
  navigation_observer.Wait();

  EXPECT_EQ(nullptr, GetCurrentInterstitial(web_contents));
}

}  // namespace

namespace decentralized_dns {

class DecentralizedDnsNavigationThrottleBrowserTest
    : public InProcessBrowserTest {
 public:
  DecentralizedDnsNavigationThrottleBrowserTest() = default;
  ~DecentralizedDnsNavigationThrottleBrowserTest() override = default;

  PrefService* local_state() { return g_browser_process->local_state(); }
};

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ShowUnstoppableDomainsInterstitialAndProceed) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("http://test.crypto")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kUnstoppableDomainsResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ENABLED),
            local_state()->GetInteger(kUnstoppableDomainsResolveMethod));
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ShowUnstoppableDomainsInterstitialAndReject) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("http://test.crypto")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kUnstoppableDomainsResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::DISABLED),
            local_state()->GetInteger(kUnstoppableDomainsResolveMethod));
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ShowENSInterstitialAndProceed) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.eth")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kENSResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ENABLED),
            local_state()->GetInteger(kENSResolveMethod));
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ShowENSInterstitialAndReject) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.eth")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kENSResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::DISABLED),
            local_state()->GetInteger(kENSResolveMethod));
}

class EnsL2OffchanLookupNavigationThrottleBrowserTest
    : public DecentralizedDnsNavigationThrottleBrowserTest {
 public:
  EnsL2OffchanLookupNavigationThrottleBrowserTest() = default;
  ~EnsL2OffchanLookupNavigationThrottleBrowserTest() override = default;

  PrefService* local_state() { return g_browser_process->local_state(); }

 protected:
  void SetEnsResolverResult(std::optional<EnsResolverTaskResult> task_result,
                            std::optional<EnsResolverTaskError> task_error) {
    ens_resolved_task_result_ = std::move(task_result);
    ens_resolved_task_error_ = std::move(task_error);
  }

  std::optional<EnsResolverTaskResult> ens_resolved_task_result_;
  std::optional<EnsResolverTaskError> ens_resolved_task_error_;
};

IN_PROC_BROWSER_TEST_F(EnsL2OffchanLookupNavigationThrottleBrowserTest,
                       ShowENSOffchainLookupInterstitialAndProceed) {
  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));
  SetEnsResolverResult(EnsResolverTaskResult({}, true), std::nullopt);

  EnsResolverTask::GetWorkOnTaskForTesting() =
      base::BindLambdaForTesting([](EnsResolverTask* task) {
        EXPECT_FALSE(task->allow_offchain().has_value());
        task->SetResultForTesting(EnsResolverTaskResult({}, true),
                                  std::nullopt);
      });

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.eth")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(EnsOffchainLookupOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EnsResolverTask::GetWorkOnTaskForTesting() = base::BindLambdaForTesting(
      [](EnsResolverTask* task) { EXPECT_TRUE(*task->allow_offchain()); });
  EXPECT_EQ(EnsOffchainResolveMethod::kAsk,
            GetEnsOffchainResolveMethod(local_state()));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
  EXPECT_EQ(EnsOffchainResolveMethod::kEnabled,
            GetEnsOffchainResolveMethod(local_state()));
}

IN_PROC_BROWSER_TEST_F(EnsL2OffchanLookupNavigationThrottleBrowserTest,
                       ShowENSOffchainLookupInterstitialAndDontProceed) {
  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ENABLED));
  SetEnsResolverResult(EnsResolverTaskResult({}, true), std::nullopt);

  EnsResolverTask::GetWorkOnTaskForTesting() =
      base::BindLambdaForTesting([](EnsResolverTask* task) {
        EXPECT_FALSE(task->allow_offchain().has_value());
        task->SetResultForTesting(EnsResolverTaskResult({}, true),
                                  std::nullopt);
      });

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.eth")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(EnsOffchainLookupOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EnsResolverTask::GetWorkOnTaskForTesting() = base::BindLambdaForTesting(
      [](EnsResolverTask* task) { EXPECT_FALSE(*task->allow_offchain()); });
  EXPECT_EQ(EnsOffchainResolveMethod::kAsk,
            GetEnsOffchainResolveMethod(local_state()));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_PROCEED);
  EXPECT_EQ(EnsOffchainResolveMethod::kDisabled,
            GetEnsOffchainResolveMethod(local_state()));
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ShowSnsInterstitialAndProceed) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.sol")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kSnsResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ENABLED),
            local_state()->GetInteger(kSnsResolveMethod));
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ShowSnsInterstitialAndReject) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.sol")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetPrimaryMainFrame()));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kSnsResolveMethod));
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_PROCEED);
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::DISABLED),
            local_state()->GetInteger(kSnsResolveMethod));
}

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottleBrowserTest,
                       ClickjackingProtectionPreventsEarlyClicks) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.eth")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* main_frame = web_contents->GetPrimaryMainFrame();

  EXPECT_TRUE(WaitForRenderFrameReady(main_frame));
  EXPECT_EQ(DecentralizedDnsOptInPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  constexpr auto* kSimulatedClickEvent =
      "document.getElementById('primary-button').click();";

  // Default resolve method is `ASK`.
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kENSResolveMethod));

  // Force clickjacking protection to be active. The 500ms timer may have
  // already expired on slow CI machines, so we explicitly set the state
  // to test the protection logic deterministically.
  EXPECT_EQ(true, content::ExecJs(main_frame, "proceedClicksEnabled = false;"));

  // Clicks while protection is active should not change the pref.
  EXPECT_EQ(true, content::ExecJs(main_frame, kSimulatedClickEvent));
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ASK),
            local_state()->GetInteger(kENSResolveMethod));

  // Enable clicks and verify the button now works.
  EXPECT_EQ(true, content::ExecJs(main_frame, "proceedClicksEnabled = true;"));
  EXPECT_EQ(true, content::ExecJs(main_frame, kSimulatedClickEvent));
  EXPECT_EQ(static_cast<int>(ResolveMethodTypes::ENABLED),
            local_state()->GetInteger(kENSResolveMethod));
}

class DecentralizedDnsNavigationThrottlePolicyTest
    : public InProcessBrowserTest {
 public:
  DecentralizedDnsNavigationThrottlePolicyTest() = default;
  ~DecentralizedDnsNavigationThrottlePolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveWalletDisabled,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_PLATFORM, base::Value(true), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

 private:
  testing::NiceMock<policy::MockConfigurationPolicyProvider> provider_;
};

IN_PROC_BROWSER_TEST_F(DecentralizedDnsNavigationThrottlePolicyTest,
                       NoInterstitialWhenWalletDisabledByPolicy) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Navigate to ENS domain - should succeed but show no interstitial
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.eth")));
  // No interstitial should be shown when wallet is disabled by policy
  EXPECT_EQ(nullptr, GetCurrentInterstitial(web_contents));

  // Navigate to Unstoppable Domains - should succeed but show no interstitial
  EXPECT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("http://test.crypto")));
  // No interstitial should be shown when wallet is disabled by policy
  EXPECT_EQ(nullptr, GetCurrentInterstitial(web_contents));

  // Navigate to SNS domain - should succeed but show no interstitial
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("http://test.sol")));
  // No interstitial should be shown when wallet is disabled by policy
  EXPECT_EQ(nullptr, GetCurrentInterstitial(web_contents));
}

}  // namespace decentralized_dns
