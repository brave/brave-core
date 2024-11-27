/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/content/decentralized_dns_opt_in_page.h"

#include <ostream>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/decentralized_dns/content/decentralized_dns_interstitial_controller_client.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/net/decentralized_dns/constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"

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
      return;
    case security_interstitials::CMD_PROCEED:
      controller()->Proceed();
      return;
  }

  NOTREACHED() << "Unsupported command: " << command;
}

void DecentralizedDnsOptInPage::PopulateInterstitialStrings(
    base::Value::Dict& load_time_data) {
  const std::u16string infura = u"Infura";
  const std::u16string infura_tou = u"https://consensys.net/terms-of-use/";
  const std::u16string infura_privacy_policy =
      u"https://consensys.net/privacy-policy/";
  const std::u16string sns_wiki_link =
      u"https://github.com/brave/brave-browser/wiki/"
      u"Resolve-Methods-for-Solana-Name-Service";
  const std::u16string sol_domain = base::ASCIIToUTF16(std::string(kSolDomain));
  const std::u16string eth_domain = base::ASCIIToUTF16(std::string(kEthDomain));

  if (IsUnstoppableDomainsTLD(request_url_.host_piece())) {
    load_time_data.Set("tabTitle", brave_l10n::GetLocalizedResourceUTF16String(
                                       IDS_UNSTOPPABLE_DOMAINS_OPT_IN_TITLE));
    load_time_data.Set("heading", brave_l10n::GetLocalizedResourceUTF16String(
                                      IDS_UNSTOPPABLE_DOMAINS_OPT_IN_HEADING));

    load_time_data.Set(
        "primaryParagraph",
        base::ReplaceStringPlaceholders(
            brave_l10n::GetLocalizedResourceUTF16String(
                IDS_UNSTOPPABLE_DOMAINS_AND_ENS_OPT_IN_PRIMARY_PARAGRAPH),
            {infura, base::ASCIIToUTF16(GetUnstoppableDomainSuffixFullList()),
             brave_l10n::GetLocalizedResourceUTF16String(
                 IDS_UNSTOPPABLE_DOMAINS_OPT_IN_TITLE),
             infura_tou, infura_privacy_policy},
            nullptr));
  } else if (IsENSTLD(request_url_.host_piece())) {
    load_time_data.Set("tabTitle", brave_l10n::GetLocalizedResourceUTF16String(
                                       IDS_ENS_OPT_IN_TITLE));
    load_time_data.Set("heading", brave_l10n::GetLocalizedResourceUTF16String(
                                      IDS_ENS_OPT_IN_HEADING));
    load_time_data.Set(
        "primaryParagraph",
        base::ReplaceStringPlaceholders(
            brave_l10n::GetLocalizedResourceUTF16String(
                IDS_UNSTOPPABLE_DOMAINS_AND_ENS_OPT_IN_PRIMARY_PARAGRAPH),
            {infura, eth_domain,
             brave_l10n::GetLocalizedResourceUTF16String(
                 IDS_ENS_DOMAIN_PROVIDER_NAME),
             infura_tou, infura_privacy_policy},
            nullptr));
  } else if (IsSnsTLD(request_url_.host_piece())) {
    load_time_data.Set("tabTitle", brave_l10n::GetLocalizedResourceUTF16String(
                                       IDS_SNS_OPT_IN_TITLE));
    load_time_data.Set("heading", brave_l10n::GetLocalizedResourceUTF16String(
                                      IDS_SNS_OPT_IN_HEADING));
    load_time_data.Set("primaryParagraph",
                       base::ReplaceStringPlaceholders(
                           brave_l10n::GetLocalizedResourceUTF16String(
                               IDS_SNS_OPT_IN_PRIMARY_PARAGRAPH),
                           {sol_domain, sns_wiki_link}, nullptr));
  } else {
    NOTREACHED();
  }

  if (IsSnsTLD(request_url_.host_piece())) {
    load_time_data.Set("primaryButtonText",
                       brave_l10n::GetLocalizedResourceUTF16String(
                           IDS_DECENTRALIZED_DNS_OPT_IN_PRIMARY_SNS_BUTTON));
  } else {
    load_time_data.Set(
        "primaryButtonText",
        base::ReplaceStringPlaceholders(
            brave_l10n::GetLocalizedResourceUTF16String(
                IDS_DECENTRALIZED_DNS_OPT_IN_PRIMARY_PROVIDER_BUTTON),
            infura, nullptr));
  }
  load_time_data.Set("dontProceedButtonText",
                     brave_l10n::GetLocalizedResourceUTF16String(
                         IDS_DECENTRALIZED_DNS_OPT_IN_DONT_PROCEED_BUTTON));
  load_time_data.Set("finalParagraph", std::u16string());
}

int DecentralizedDnsOptInPage::GetHTMLTemplateId() {
  return IDR_DECENTRALIZED_DNS_INTERSTITIAL_HTML;
}

security_interstitials::SecurityInterstitialPage::TypeID
DecentralizedDnsOptInPage::GetTypeForTesting() {
  return DecentralizedDnsOptInPage::kTypeForTesting;
}

}  // namespace decentralized_dns
