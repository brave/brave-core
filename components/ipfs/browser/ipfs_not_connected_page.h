/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_NOT_CONNECTED_PAGE_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_NOT_CONNECTED_PAGE_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace security_interstitials {
class SecurityInterstitialControllerClient;
}  // namespace security_interstitials

namespace ipfs {

class IPFSNotConnectedPage
    : public security_interstitials::SecurityInterstitialPage {
 public:
  // Interstitial type, used in tests.
  static const security_interstitials::SecurityInterstitialPage::TypeID
      kTypeForTesting;

  IPFSNotConnectedPage(
      content::WebContents* web_contents,
      const GURL& request_url,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller);
  ~IPFSNotConnectedPage() override;

  IPFSNotConnectedPage(const IPFSNotConnectedPage&) = delete;
  IPFSNotConnectedPage& operator=(const IPFSNotConnectedPage&) = delete;

  // SecurityInterstitialPage::
  void OnInterstitialClosing() override {}
  void CommandReceived(const std::string& command) override;
  security_interstitials::SecurityInterstitialPage::TypeID GetTypeForTesting()
      override;

 protected:
  // SecurityInterstitialPage::
  void PopulateInterstitialStrings(
      base::DictionaryValue* load_time_data) override;
  int GetHTMLTemplateId() override;

 private:
  friend class IpfsNavigationThrottleBrowserTest;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_NOT_CONNECTED_PAGE_H_
