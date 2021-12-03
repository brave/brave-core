/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_not_connected_page.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/security_interstitials/core/controller_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

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

  EXPECT_EQ(ipfs::IPFSNotConnectedPage::kTypeForTesting,
            GetInterstitialType(web_contents));

  content::TestNavigationObserver navigation_observer(web_contents, 1);
  SendInterstitialCommand(web_contents, command);
  navigation_observer.Wait();

  EXPECT_EQ(nullptr, GetCurrentInterstitial(web_contents));
}

}  // namespace

namespace ipfs {

class IpfsNavigationThrottleBrowserTest : public InProcessBrowserTest {
 public:
  IpfsNavigationThrottleBrowserTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  ~IpfsNavigationThrottleBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ipfs_service_ =
        IpfsServiceFactory::GetInstance()->GetForContext(browser()->profile());
    ASSERT_TRUE(ipfs_service_);
    ipfs_service_->SetAllowIpfsLaunchForTest(true);

    ipfs_url_ = GURL(
        "http://127.0.0.1:48080/ipfs/"
        "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
    gateway_url_ = GURL(
        "https://dweb.link/ipfs/"
        "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");

    InProcessBrowserTest::SetUpOnMainThread();
  }

  void ResetTestServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    test_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    test_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    test_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(test_server_->Start());
    ipfs_service_->SetServerEndpointForTest(test_server_->base_url());
  }

  std::unique_ptr<net::test_server::HttpResponse> GetConnectedPeersResponse(
      const net::test_server::HttpRequest& request,
      bool empty) {
    if (request.GetURL().path_piece() != kSwarmPeersPath) {
      return nullptr;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    if (empty) {
      http_response->set_content(R"({})");
    } else {
      http_response->set_content(R"({
        "Peers": [
          {
            "Addr": "/ip4/101.101.101.101/tcp/4001",
            "Direction": 0,
            "Peer": "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"
          },
          {
            "Addr": "/ip4/102.102.102.102/tcp/4001",
            "Direction": 0,
            "Peer": "QmStjfkGsfQGQQm6Gdxin6DvrZsFTmTNoX5oEFMzYrc1PS"
          }
        ]
      })");
    }

    return http_response;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleGetEmptyConnectedPeers(
      const net::test_server::HttpRequest& request) {
    return GetConnectedPeersResponse(request, true);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleGetConnectedPeers(
      const net::test_server::HttpRequest& request) {
    return GetConnectedPeersResponse(request, false);
  }

  IpfsService* ipfs_service() { return ipfs_service_; }
  PrefService* GetPrefs() const { return browser()->profile()->GetPrefs(); }
  const GURL& ipfs_url() { return ipfs_url_; }
  const GURL& gateway_url() { return gateway_url_; }

 private:
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  raw_ptr<IpfsService> ipfs_service_ = nullptr;
  base::test::ScopedFeatureList feature_list_;
  GURL ipfs_url_;
  GURL gateway_url_;
};

IN_PROC_BROWSER_TEST_F(IpfsNavigationThrottleBrowserTest,
                       ShowInterstitialForEmptyConnectedPeers) {
  ResetTestServer(base::BindRepeating(
      &IpfsNavigationThrottleBrowserTest::HandleGetEmptyConnectedPeers,
      base::Unretained(this)));

  GetPrefs()->SetInteger(kIPFSResolveMethod,
                         static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  // Navigate to IPFS URL and check if we'll show the interstitial when there
  // are no connected peers.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetMainFrame()));
  EXPECT_EQ(IPFSNotConnectedPage::kTypeForTesting,
            GetInterstitialType(web_contents));
  EXPECT_FALSE(GetPrefs()->GetBoolean(kIPFSAutoFallbackToGateway));

  // Send Proceed command and check if we fallback to gateway and pref is set.
  SendInterstitialCommandSync(
      browser(),
      security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
  EXPECT_EQ(gateway_url(), web_contents->GetURL());
  EXPECT_TRUE(GetPrefs()->GetBoolean(kIPFSAutoFallbackToGateway));

  // Navigate to that URL again and see if we auto fallback to gateway this
  // time without interstitials.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetMainFrame()));
  EXPECT_EQ(nullptr, GetInterstitialType(web_contents));
  EXPECT_EQ(gateway_url(), web_contents->GetURL());
}

IN_PROC_BROWSER_TEST_F(IpfsNavigationThrottleBrowserTest,
                       NoInterstitialShownForNonEmptyConnectedPeers) {
  ResetTestServer(base::BindRepeating(
      &IpfsNavigationThrottleBrowserTest::HandleGetConnectedPeers,
      base::Unretained(this)));

  EXPECT_FALSE(GetPrefs()->GetBoolean(kIPFSAutoFallbackToGateway));
  GetPrefs()->SetInteger(kIPFSResolveMethod,
                         static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_TRUE(WaitForRenderFrameReady(web_contents->GetMainFrame()));
  EXPECT_EQ(nullptr, GetInterstitialType(web_contents));
}

}  // namespace ipfs
