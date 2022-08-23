/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/navigation_handle_observer.h"
#include "net/base/net_errors.h"

namespace ipfs {

class IpfsRedirectNetworkDelegateHelperBrowserTest
    : public InProcessBrowserTest {
 public:
  IpfsRedirectNetworkDelegateHelperBrowserTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  ~IpfsRedirectNetworkDelegateHelperBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ipfs_url_ = GURL("ipfs://QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
    gateway_url_ = GURL(
        "https://dweb.link/ipfs/"
        "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
  }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  PrefService* GetPrefs() const { return browser()->profile()->GetPrefs(); }
  const GURL& ipfs_url() { return ipfs_url_; }
  const GURL& gateway_url() { return gateway_url_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  GURL ipfs_url_;
  GURL gateway_url_;
};

IN_PROC_BROWSER_TEST_F(IpfsRedirectNetworkDelegateHelperBrowserTest,
                       IPFSResolveMethodDisabledNoRedirect) {
  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));

  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  EXPECT_EQ(web_contents()->GetURL(), ipfs_url());
}

IN_PROC_BROWSER_TEST_F(IpfsRedirectNetworkDelegateHelperBrowserTest,
                       IPFSResolveMethodGatewayRedirect) {
  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  EXPECT_EQ(web_contents()->GetURL(), gateway_url());
}

IN_PROC_BROWSER_TEST_F(IpfsRedirectNetworkDelegateHelperBrowserTest,
                       IPFSResolveRedirectsToErrorPage_Incognito) {
  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  EXPECT_EQ(web_contents()->GetURL(), gateway_url());

  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  auto* private_wc = private_browser->tab_strip_model()->GetActiveWebContents();

  content::NavigationHandleObserver observer(private_wc, ipfs_url());

  // Try to navigate to the url. The navigation should be canceled and the
  // NavigationHandle should have the right error code.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(private_browser, ipfs_url()));
  EXPECT_TRUE(private_wc->GetPrimaryMainFrame()->IsErrorDocument());
  EXPECT_EQ(net::ERR_INCOGNITO_IPFS_NOT_ALLOWED, observer.net_error_code());
}

IN_PROC_BROWSER_TEST_F(IpfsRedirectNetworkDelegateHelperBrowserTest,
                       IPFSResolveRedirectsToErrorPage_IpfsDisabled) {
  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));

  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  EXPECT_EQ(web_contents()->GetURL(), ipfs_url());

  auto* wc = browser()->tab_strip_model()->GetActiveWebContents();

  content::NavigationHandleObserver observer(wc, ipfs_url());

  // Try to navigate to the url. The navigation should be canceled and the
  // NavigationHandle should have the right error code.
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), ipfs_url()));
  EXPECT_TRUE(wc->GetPrimaryMainFrame()->IsErrorDocument());
  EXPECT_EQ(net::ERR_IPFS_DISABLED, observer.net_error_code());
}

}  // namespace ipfs
