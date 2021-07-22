/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/domain_block_page.h"

#include <ostream>
#include <utility>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_shields/browser/domain_block_controller_client.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_shields {

// static
const security_interstitials::SecurityInterstitialPage::TypeID
    DomainBlockPage::kTypeForTesting = &DomainBlockPage::kTypeForTesting;

DomainBlockPage::DomainBlockPage(
    content::WebContents* web_contents,
    const GURL& request_url,
    std::unique_ptr<
        security_interstitials::SecurityInterstitialControllerClient>
        controller)
    : security_interstitials::SecurityInterstitialPage(web_contents,
                                                       request_url,
                                                       std::move(controller)) {}

DomainBlockPage::~DomainBlockPage() = default;

void DomainBlockPage::CommandReceived(const std::string& command) {
  if (command == "\"pageLoadComplete\"") {
    // content::WaitForRenderFrameReady sends this message when the page
    // load completes. Ignore it.
    return;
  }

  int cmd = 0;
  bool retval = base::StringToInt(command, &cmd);
  DCHECK(retval);
  DomainBlockControllerClient* domain_block_controller =
      static_cast<DomainBlockControllerClient*>(controller());

  switch (cmd) {
    case security_interstitials::CMD_DONT_PROCEED:
      domain_block_controller->GoBack();
      break;
    case security_interstitials::CMD_PROCEED:
      domain_block_controller->Proceed();
      break;
    case security_interstitials::CMD_DO_REPORT:
      domain_block_controller->SetDontWarnAgain(true);
      break;
    case security_interstitials::CMD_DONT_REPORT:
      domain_block_controller->SetDontWarnAgain(false);
      break;
    default:
      NOTREACHED() << "Unsupported command: " << command;
  }
}

void DomainBlockPage::PopulateInterstitialStrings(base::Value* load_time_data) {
  load_time_data->SetStringKey(
      "tabTitle", l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_TITLE));
  load_time_data->SetStringKey(
      "heading", l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_HEADING));

  load_time_data->SetStringKey(
      "primaryParagraph",
      l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_PRIMARY_PARAGRAPH));
  load_time_data->SetStringKey("domain", request_url().GetOrigin().spec());
  load_time_data->SetStringKey(
      "explanationParagraph",
      l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_EXPLANATION));

  load_time_data->SetStringKey(
      "dontWarnAgainText",
      l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_DONT_WARN_AGAIN_BUTTON));

  load_time_data->SetStringKey(
      "proceedAnywayText",
      l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_PROCEED_ANYWAY_BUTTON));
  load_time_data->SetStringKey(
      "goBackText", l10n_util::GetStringUTF16(IDS_DOMAIN_BLOCK_GO_BACK_BUTTON));
}

int DomainBlockPage::GetHTMLTemplateId() {
  return IDR_DOMAIN_BLOCK_INTERSTITIAL_HTML;
}

security_interstitials::SecurityInterstitialPage::TypeID
DomainBlockPage::GetTypeForTesting() {
  return DomainBlockPage::kTypeForTesting;
}

}  // namespace brave_shields
