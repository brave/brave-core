/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_PAGE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_PAGE_H_

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

namespace brave_shields {

// DomainBlockPage is the interstitial page which will be shown when the
// browser blocks a top-level, first-party request. A proceed button is
// provided in the page, along with a checkbox to whitelist the domain and
// bypass this interstitial for this domain in the future.
class DomainBlockPage
    : public security_interstitials::SecurityInterstitialPage {
 public:
  // Interstitial type, used in tests.
  static const security_interstitials::SecurityInterstitialPage::TypeID
      kTypeForTesting;

  DomainBlockPage(
      content::WebContents* web_contents,
      const GURL& request_url,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller);
  ~DomainBlockPage() override;

  DomainBlockPage(const DomainBlockPage&) = delete;
  DomainBlockPage& operator=(const DomainBlockPage&) = delete;

  // SecurityInterstitialPage:: overrides
  void OnInterstitialClosing() override {}
  void CommandReceived(const std::string& command) override;
  security_interstitials::SecurityInterstitialPage::TypeID GetTypeForTesting()
      override;

 protected:
  // SecurityInterstitialPage:: overrides
  void PopulateInterstitialStrings(base::Value* load_time_data) override;
  int GetHTMLTemplateId() override;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_PAGE_H_
