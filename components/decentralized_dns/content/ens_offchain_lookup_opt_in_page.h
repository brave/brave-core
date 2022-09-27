/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONTENT_ENS_OFFCHAIN_LOOKUP_OPT_IN_PAGE_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONTENT_ENS_OFFCHAIN_LOOKUP_OPT_IN_PAGE_H_

#include <memory>
#include <string>

#include "components/security_interstitials/content/security_interstitial_page.h"

namespace decentralized_dns {

class EnsOffchainLookupOptInPage
    : public security_interstitials::SecurityInterstitialPage {
 public:
  // Interstitial type, used in tests.
  static const security_interstitials::SecurityInterstitialPage::TypeID
      kTypeForTesting;

  EnsOffchainLookupOptInPage(
      content::WebContents* web_contents,
      const GURL& request_url,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller);
  ~EnsOffchainLookupOptInPage() override;

  EnsOffchainLookupOptInPage(const EnsOffchainLookupOptInPage&) = delete;
  EnsOffchainLookupOptInPage& operator=(const EnsOffchainLookupOptInPage&) =
      delete;

 protected:
  // SecurityInterstitialPage::
  void PopulateInterstitialStrings(base::Value::Dict& load_time_data) override;
  int GetHTMLTemplateId() override;
  void OnInterstitialClosing() override {}
  void CommandReceived(const std::string& command) override;
  security_interstitials::SecurityInterstitialPage::TypeID GetTypeForTesting()
      override;

 private:
  const GURL request_url_;
};

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONTENT_ENS_OFFCHAIN_LOOKUP_OPT_IN_PAGE_H_
