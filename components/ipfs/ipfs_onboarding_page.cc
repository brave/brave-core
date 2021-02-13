/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_onboarding_page.h"

#include <utility>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/isolated_world_ids.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
const char kResponseScript[] =
    "window.postMessage({command: 'ipfs-error', value: {value}}, '*')";
const char kBraveSettingsURL[] = "brave://settings/ipfs";
constexpr int kOnboardingIsolatedWorldId =
    content::ISOLATED_WORLD_ID_CONTENT_END + 1;
}  // namespace
namespace ipfs {

// static
const security_interstitials::SecurityInterstitialPage::TypeID
    IPFSOnboardingPage::kTypeForTesting = &IPFSOnboardingPage::kTypeForTesting;

IPFSOnboardingPage::IPFSOnboardingPage(
    IpfsService* ipfs_service,
    content::WebContents* web_contents,
    const GURL& request_url,
    std::unique_ptr<
        security_interstitials::SecurityInterstitialControllerClient>
        controller)
    : security_interstitials::SecurityInterstitialPage(web_contents,
                                                       request_url,
                                                       std::move(controller)),
      ipfs_service_(ipfs_service) {}

IPFSOnboardingPage::~IPFSOnboardingPage() = default;

void IPFSOnboardingPage::UseLocalNode() {
  auto* context = web_contents()->GetBrowserContext();
  auto* prefs = user_prefs::UserPrefs::Get(context);
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  ipfs_service_->LaunchDaemon(base::BindOnce(
      &IPFSOnboardingPage::OnIpfsLaunched, weak_ptr_factory_.GetWeakPtr()));
}

void IPFSOnboardingPage::UsePublicGateway() {
  auto* prefs = user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  Proceed();
}

void IPFSOnboardingPage::OnIpfsLaunched(bool result) {
  if (!result) {
    RespondToPage(LOCAL_NODE_ERROR);
    return;
  }
  Proceed();
}

void IPFSOnboardingPage::Proceed() {
  controller()->OpenUrlInCurrentTab(request_url());
}

void IPFSOnboardingPage::RespondToPage(IPFSOnboardingResponse value) {
  auto* main_frame = web_contents()->GetMainFrame();
  DCHECK(main_frame);

  std::string script(kResponseScript);
  base::ReplaceSubstringsAfterOffset(&script, 0, "{value}",
                                     std::to_string(value));
  main_frame->ExecuteJavaScriptInIsolatedWorld(base::ASCIIToUTF16(script), {},
                                               kOnboardingIsolatedWorldId);
}

void IPFSOnboardingPage::CommandReceived(const std::string& command) {
  if (command == "\"pageLoadComplete\"") {
    // content::WaitForRenderFrameReady sends this message when the page
    // load completes. Ignore it.
    return;
  }

  int cmd = 0;
  bool retval = base::StringToInt(command, &cmd);
  DCHECK(retval);

  switch (cmd) {
    case IPFSOnboardingCommandId::USE_LOCAL_NODE:
      UseLocalNode();
      break;
    case IPFSOnboardingCommandId::USE_PUBLIC_GATEWAY:
      UsePublicGateway();
      break;
    case IPFSOnboardingCommandId::LEARN_MORE:
      controller()->OpenUrlInCurrentTab(GURL(kIPFSLearnMoreURL));
      break;
    case IPFSOnboardingCommandId::OPEN_SETTINGS:
      controller()->OpenUrlInCurrentTab(GURL(kBraveSettingsURL));
      break;
    default:
      NOTREACHED() << "Unsupported command: " << command;
  }
}

void IPFSOnboardingPage::PopulateInterstitialStrings(
    base::DictionaryValue* load_time_data) {
  load_time_data->SetString(
      "tabTitle", l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_TITLE));
  load_time_data->SetString(
      "heading", l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_HEADING));
  load_time_data->SetString(
      "primaryParagraph",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_PRIMARY_PARAGRAPH));
  load_time_data->SetString(
      "localNodeText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_LOCAL_NODE_BUTTON));
  load_time_data->SetString(
      "publicGatewayText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_PUBLIC_GATEWAY_BUTTON));
}

int IPFSOnboardingPage::GetHTMLTemplateId() {
  return IDR_IPFS_INTERSTITIAL_ONBOARDING_HTML;
}

security_interstitials::SecurityInterstitialPage::TypeID
IPFSOnboardingPage::GetTypeForTesting() {
  return IPFSOnboardingPage::kTypeForTesting;
}

}  // namespace ipfs
