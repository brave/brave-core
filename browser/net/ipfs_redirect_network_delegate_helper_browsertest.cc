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
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"

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
        "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR/");
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

}  // namespace ipfs
