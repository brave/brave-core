/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/unstoppable_domains_opt_in_page.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/unstoppable_domains/unstoppable_domains_interstitial_controller_client.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "ui/base/l10n/l10n_util.h"

namespace unstoppable_domains {

// static
const security_interstitials::SecurityInterstitialPage::TypeID
    UnstoppableDomainsOptInPage::kTypeForTesting =
        &UnstoppableDomainsOptInPage::kTypeForTesting;

UnstoppableDomainsOptInPage::UnstoppableDomainsOptInPage(
    content::WebContents* web_contents,
    const GURL& request_url,
    std::unique_ptr<
        security_interstitials::SecurityInterstitialControllerClient>
        controller)
    : security_interstitials::SecurityInterstitialPage(web_contents,
                                                       request_url,
                                                       std::move(controller)) {}

UnstoppableDomainsOptInPage::~UnstoppableDomainsOptInPage() = default;

void UnstoppableDomainsOptInPage::CommandReceived(const std::string& command) {
  if (command == "\"pageLoadComplete\"") {
    // content::WaitForRenderFrameReady sends this message when the page
    // load completes. Ignore it.
    return;
  }

  int cmd = 0;
  bool retval = base::StringToInt(command, &cmd);
  DCHECK(retval);

  switch (cmd) {
    case security_interstitials::CMD_DONT_PROCEED:
      static_cast<UnstoppableDomainsInterstitialControllerClient*>(controller())
          ->DontProceed();
      break;
    case security_interstitials::CMD_PROCEED:
      controller()->Proceed();
      break;
    default:
      NOTREACHED() << "Unsupported command: " << command;
  }
}

void UnstoppableDomainsOptInPage::PopulateInterstitialStrings(
    base::DictionaryValue* load_time_data) {
  load_time_data->SetString(
      "tabTitle",
      l10n_util::GetStringUTF16(IDS_UNSTOPPABLE_DOMAINS_OPT_IN_TITLE));
  load_time_data->SetString(
      "heading",
      l10n_util::GetStringUTF16(IDS_UNSTOPPABLE_DOMAINS_OPT_IN_HEADING));

  const std::vector<base::string16> message_params = {
      base::ASCIIToUTF16("<a "
                         "href='https://www.cloudflare.com/en-ca/"
                         "distributed-web-gateway-terms/' target='_blank' "
                         "rel='noopener noreferrer'>"),
      base::ASCIIToUTF16("</a>"),
      base::ASCIIToUTF16(
          "<a href='https://www.cloudflare.com/en-ca/privacypolicy/' "
          "target='_blank' rel='noopener noreferrer'>"),
      base::ASCIIToUTF16("</a>"),
  };
  load_time_data->SetString(
      "primaryParagraph",
      base::ReplaceStringPlaceholders(
          l10n_util::GetStringUTF16(
              IDS_UNSTOPPABLE_DOMAINS_OPT_IN_PRIMARY_PARAGRAPH),
          message_params, nullptr));

  load_time_data->SetString(
      "primaryButtonText",
      l10n_util::GetStringUTF16(IDS_UNSTOPPABLE_DOMAINS_OPT_IN_PRIMARY_BUTTON));
  load_time_data->SetString(
      "dontProceedButtonText",
      l10n_util::GetStringUTF16(
          IDS_UNSTOPPABLE_DOMAINS_OPT_IN_DONT_PROCEED_BUTTON));
  load_time_data->SetString("finalParagraph", base::string16());
}

int UnstoppableDomainsOptInPage::GetHTMLTemplateId() {
  return IDR_UNSTOPPABLE_DOMAINS_INTERSTITIAL_HTML;
}

security_interstitials::SecurityInterstitialPage::TypeID
UnstoppableDomainsOptInPage::GetTypeForTesting() {
  return UnstoppableDomainsOptInPage::kTypeForTesting;
}

}  // namespace unstoppable_domains
