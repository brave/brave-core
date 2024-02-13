/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_BLOCKING_PAGE_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_BLOCKING_PAGE_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "url/gurl.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace security_interstitials {
class SecurityInterstitialControllerClient;
}  // namespace security_interstitials

namespace request_otr {

// RequestOTRBlockingPage is the interstitial page which will be shown when the
// browser blocks a top-level, first-party request. A proceed button is
// provided in the page, along with a checkbox to bypass this interstitial in
// the future.
class RequestOTRBlockingPage
    : public security_interstitials::SecurityInterstitialPage {
 public:
  // Interstitial type, used in tests.
  static const security_interstitials::SecurityInterstitialPage::TypeID
      kTypeForTesting;

  RequestOTRBlockingPage(
      content::WebContents* web_contents,
      const GURL& request_url,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller);
  ~RequestOTRBlockingPage() override;

  RequestOTRBlockingPage(const RequestOTRBlockingPage&) = delete;
  RequestOTRBlockingPage& operator=(const RequestOTRBlockingPage&) = delete;

  // SecurityInterstitialPage:: overrides
  void OnInterstitialClosing() override;
  void CommandReceived(const std::string& command) override;
  security_interstitials::SecurityInterstitialPage::TypeID GetTypeForTesting()
      override;

 protected:
  // SecurityInterstitialPage:: overrides
  void PopulateInterstitialStrings(base::Value::Dict& load_time_data) override;
  int GetHTMLTemplateId() override;

 private:
  base::Time start_time_;
  raw_ptr<PrefService> profile_prefs_;
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_BLOCKING_PAGE_H_
