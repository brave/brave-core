/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsPublisherBrowserTest.*

namespace brave_rewards {

class RewardsPublisherBrowserTest : public InProcessBrowserTest {
 public:
  RewardsPublisherBrowserTest() {
    response_ = std::make_unique<test_util::RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<test_util::RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&test_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsPublisherBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetEngineEnvForTesting();

    test_util::SetOnboardingBypassed(browser());
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      base::flat_map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  raw_ptr<RewardsServiceImpl, DanglingUntriaged> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<test_util::RewardsBrowserTestResponse> response_;
  std::unique_ptr<test_util::RewardsBrowserTestContextHelper> context_helper_;
};

IN_PROC_BROWSER_TEST_F(RewardsPublisherBrowserTest,
                       PanelShowsCorrectPublisherData) {
  test_util::StartProcessWithConnectedUser(browser()->profile());
  // Navigate to a verified site in a new tab
  const std::string publisher = "duckduckgo.com";
  test_util::NavigateToPublisherAndWaitForUpdate(browser(), https_server_.get(),
                                                 publisher);

  // Open the Rewards popup
  base::WeakPtr<content::WebContents> popup_contents =
      context_helper_->OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  // Retrieve the inner text of the wallet panel and verify that it
  // looks as expected
  std::string card_selector = "[data-test-id=publisher-card]";
  test_util::WaitForElementToContain(popup_contents.get(), card_selector,
                                     "Verified Creator");
  test_util::WaitForElementToContain(popup_contents.get(), card_selector,
                                     publisher);

  // Retrieve the inner HTML of the wallet panel and verify that it
  // contains the expected favicon
  {
    const std::string favicon =
        "chrome://favicon2/?"
        "size=64&amp;"
        "pageUrl=https%3A%2F%2Fduckduckgo.com%2F";
    test_util::WaitForElementToContainHTML(popup_contents.get(), card_selector,
                                           favicon);
  }
}

}  // namespace brave_rewards
