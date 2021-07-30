/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/decentralized_dns_opt_in_page.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/decentralized_dns/decentralized_dns_interstitial_controller_client.h"
#include "brave/components/decentralized_dns/utils.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "ui/base/l10n/l10n_util.h"

namespace decentralized_dns {

// static
const security_interstitials::SecurityInterstitialPage::TypeID
    DecentralizedDnsOptInPage::kTypeForTesting =
        &DecentralizedDnsOptInPage::kTypeForTesting;

DecentralizedDnsOptInPage::DecentralizedDnsOptInPage(
    content::WebContents* web_contents,
    const GURL& request_url,
    std::unique_ptr<
        security_interstitials::SecurityInterstitialControllerClient>
        controller)
    : security_interstitials::SecurityInterstitialPage(web_contents,
                                                       request_url,
                                                       std::move(controller)),
      request_url_(request_url) {}

DecentralizedDnsOptInPage::~DecentralizedDnsOptInPage() = default;

void DecentralizedDnsOptInPage::CommandReceived(const std::string& command) {
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
      static_cast<DecentralizedDnsInterstitialControllerClient*>(controller())
          ->DontProceed();
      break;
    case security_interstitials::CMD_PROCEED:
      controller()->Proceed();
      break;
    default:
      NOTREACHED() << "Unsupported command: " << command;
  }
}

void DecentralizedDnsOptInPage::PopulateInterstitialStrings(
    base::DictionaryValue* load_time_data) {
  const std::vector<std::u16string> message_params = {
      u"<a "
      u"href='https://www.cloudflare.com/en-ca/"
      u"distributed-web-gateway-terms/' target='_blank' "
      u"rel='noopener noreferrer'>",
      u"</a>",
      u"<a href='https://developers.cloudflare.com/"
      u"1.1.1.1/privacy/public-dns-resolver' "
      u"target='_blank' rel='noopener noreferrer'>",
      u"</a>",
  };

  if (IsUnstoppableDomainsTLD(request_url_)) {
    load_time_data->SetString(
        "tabTitle",
        l10n_util::GetStringUTF16(IDS_UNSTOPPABLE_DOMAINS_OPT_IN_TITLE));
    load_time_data->SetString(
        "heading",
        l10n_util::GetStringUTF16(IDS_UNSTOPPABLE_DOMAINS_OPT_IN_HEADING));

    load_time_data->SetString(
        "primaryParagraph",
        base::ReplaceStringPlaceholders(
            l10n_util::GetStringUTF16(
                IDS_UNSTOPPABLE_DOMAINS_OPT_IN_PRIMARY_PARAGRAPH),
            message_params, nullptr));
  } else {
    load_time_data->SetString("tabTitle",
                              l10n_util::GetStringUTF16(IDS_ENS_OPT_IN_TITLE));
    load_time_data->SetString(
        "heading", l10n_util::GetStringUTF16(IDS_ENS_OPT_IN_HEADING));
    load_time_data->SetString(
        "primaryParagraph",
        base::ReplaceStringPlaceholders(
            l10n_util::GetStringUTF16(IDS_ENS_OPT_IN_PRIMARY_PARAGRAPH),
            message_params, nullptr));
  }

  load_time_data->SetString(
      "primaryButtonText",
      l10n_util::GetStringUTF16(IDS_DECENTRALIZED_DNS_OPT_IN_PRIMARY_BUTTON));
  load_time_data->SetString(
      "dontProceedButtonText",
      l10n_util::GetStringUTF16(
          IDS_DECENTRALIZED_DNS_OPT_IN_DONT_PROCEED_BUTTON));
  load_time_data->SetString("finalParagraph", std::u16string());
}

int DecentralizedDnsOptInPage::GetHTMLTemplateId() {
  return IDR_DECENTRALIZED_DNS_INTERSTITIAL_HTML;
}

security_interstitials::SecurityInterstitialPage::TypeID
DecentralizedDnsOptInPage::GetTypeForTesting() {
  return DecentralizedDnsOptInPage::kTypeForTesting;
}

}  // namespace decentralized_dns
