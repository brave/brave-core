/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsPublisherBrowserTest.*

namespace rewards_browsertest {

class RewardsPublisherBrowserTest : public InProcessBrowserTest {
 public:
  RewardsPublisherBrowserTest() {
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&rewards_browsertest_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsPublisherBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();
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
      std::map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
};

IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    PanelShowsCorrectPublisherData) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to a verified site in a new tab
  const std::string publisher = "duckduckgo.com";
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      publisher);

  // Open the Rewards popup
  content::WebContents* popup_contents =
      rewards_browsertest_helper::OpenRewardsPopup(browser());
  ASSERT_TRUE(popup_contents);

  // Retrieve the inner text of the wallet panel and verify that it
  // looks as expected
  rewards_browsertest_util::WaitForElementToContain(
      popup_contents,
      "[id='wallet-panel']",
      "Brave Verified Creator");
  rewards_browsertest_util::WaitForElementToContain(
      popup_contents,
      "[id='wallet-panel']",
      publisher);

  // Retrieve the inner HTML of the wallet panel and verify that it
  // contains the expected favicon
  {
    const std::string favicon =
        "chrome://favicon/size/64@1x/https://" + publisher;
    rewards_browsertest_util::WaitForElementToContainHTML(
        popup_contents,
        "#wallet-panel",
        favicon);
  }
}

IN_PROC_BROWSER_TEST_F(RewardsPublisherBrowserTest, VisitVerifiedPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "duckduckgo.com"),
      true);
}

IN_PROC_BROWSER_TEST_F(RewardsPublisherBrowserTest, VisitUnverifiedPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(https_server_.get(), "brave.com"),
      false);
}

// Registered publishers without a wallet address are displayed as verified
IN_PROC_BROWSER_TEST_F(RewardsPublisherBrowserTest, VisitRegisteredPublisher) {
  rewards_browsertest_helper::EnableRewards(browser());

  rewards_browsertest_helper::VisitPublisher(
      browser(),
      rewards_browsertest_util::GetUrl(
          https_server_.get(),
          "registeredsite.com"),
      true);
}

// Brave tip icon is injected when visiting Twitter
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    TwitterTipsInjectedOnTwitter) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to Twitter in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "twitter.com",
      "/twitter");

  // Ensure that Media tips injection is active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting Twitter while Brave
// Rewards is disabled
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    TwitterTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to Twitter in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "twitter.com",
      "/twitter");

  // Ensure that Media tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is injected when visiting old Twitter
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    TwitterTipsInjectedOnOldTwitter) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to Twitter in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "twitter.com",
      "/oldtwitter");

  // Ensure that Media tips injection is active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting old Twitter while
// Brave Rewards is disabled
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    TwitterTipsNotInjectedWhenRewardsDisabledOldTwitter) {
  // Navigate to Twitter in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "twitter.com",
      "/oldtwitter");

  // Ensure that Media tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is not injected into non-Twitter sites
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    TwitterTipsNotInjectedOnNonTwitter) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to a non-Twitter site in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "brave.com");

  // Ensure that Media tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    RedditTipsInjectedOnReddit) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to Reddit in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "reddit.com",
      "/reddit");

  // Ensure that Media Tips injection is active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    RedditTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to Reddit in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "reddit.com",
      "/reddit");

  // Ensure that Media Tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is not injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    RedditTipsNotInjectedOnNonReddit) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to Reddit in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "brave.com",
      "/reddit");

  // Ensure that Media Tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is injected when visiting GitHub
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    GitHubTipsInjectedOnGitHub) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to GitHub in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "github.com",
      "/github");

  // Ensure that Media Tips injection is active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting GitHub while Brave
// Rewards is disabled
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    GitHubTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to GitHub in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "github.com",
      "/github");

  // Ensure that Media Tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is not injected when not visiting GitHub
IN_PROC_BROWSER_TEST_F(
    RewardsPublisherBrowserTest,
    GitHubTipsNotInjectedOnNonGitHub) {
  rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);

  // Navigate to GitHub in a new tab
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "brave.com",
      "/github");

  // Ensure that Media Tips injection is not active
  rewards_browsertest_util::IsMediaTipsInjected(contents(), false);
}

}  // namespace rewards_browsertest
