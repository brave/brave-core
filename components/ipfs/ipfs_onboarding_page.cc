/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_onboarding_page.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/component_updater/component_updater_service.h"
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
    "window.postMessage({command: 'ipfs', value: {value}, text: '{text}'}, "
    "'*')";
const char kBraveSettingsURL[] = "brave://settings/ipfs";
constexpr int kOnboardingIsolatedWorldId =
    content::ISOLATED_WORLD_ID_CONTENT_END + 1;

// The period in seconds during which we will repeat requests
// to get connected peers if no peers available
constexpr int kConnectedPeersRetryLimitSec = 120;

// The period in seconds between requests to get connected peers information.
constexpr int kConnectedPeersRetryStepSec = 1;

// The period in seconds when we alert user about error.
constexpr int kConnectedPeersAlertTimeoutSec = 10;
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
      ipfs_service_(ipfs_service) {
  service_observer_.Observe(ipfs_service_);
  theme_observer_.Observe(ui::NativeTheme::GetInstanceForNativeUi());
}

IPFSOnboardingPage::~IPFSOnboardingPage() = default;

void IPFSOnboardingPage::UseLocalNode() {
  auto* context = web_contents()->GetBrowserContext();
  auto* prefs = user_prefs::UserPrefs::Get(context);
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  start_time_ticks_ = base::TimeTicks::Now();
  if (!ipfs_service_->IsDaemonLaunched()) {
    ipfs_service_->LaunchDaemon(base::NullCallback());
  } else {
    RespondToPage(LOCAL_NODE_LAUNCHED, base::string16());
    GetConnectedPeers();
  }
}

void IPFSOnboardingPage::UsePublicGateway() {
  auto* prefs = user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  Proceed();
}

void IPFSOnboardingPage::OnIpfsShutdown() {
  ReportDaemonStopped();
}

void IPFSOnboardingPage::OnInstallationEvent(
    ipfs::ComponentUpdaterEvents event) {
  if (event == ipfs::ComponentUpdaterEvents::COMPONENT_UPDATE_ERROR) {
    RespondToPage(
        INSTALLATION_ERROR,
        l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_INSTALLATION_ERROR));
  }
}

void IPFSOnboardingPage::OnGetConnectedPeers(
    bool success,
    const std::vector<std::string>& peers) {
  if (!success || peers.empty()) {
    base::TimeDelta delta = base::TimeTicks::Now() - start_time_ticks_;
    if (delta.InSeconds() < kConnectedPeersRetryLimitSec) {
      if (delta.InSeconds() > kConnectedPeersAlertTimeoutSec) {
        int retries = (kConnectedPeersRetryLimitSec - delta.InSeconds()) /
                      kConnectedPeersRetryStepSec;
        RespondToPage(NO_PEERS_AVAILABLE, l10n_util::GetStringFUTF16(
                                              IDS_IPFS_ONBOARDING_PEERS_ERROR,
                                              base::NumberToString16(retries)));
      }
      base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&IPFSOnboardingPage::GetConnectedPeers,
                         weak_ptr_factory_.GetWeakPtr()),
          base::TimeDelta::FromSeconds(kConnectedPeersRetryStepSec));
    } else {
      RespondToPage(NO_PEERS_LIMIT, l10n_util::GetStringUTF16(
                                        IDS_IPFS_ONBOARDING_PEERS_LIMIT_ERROR));
    }
    return;
  }
  if (IsLocalNodeMode()) {
    Proceed();
  }
}

void IPFSOnboardingPage::ReportDaemonStopped() {
  RespondToPage(LOCAL_NODE_ERROR,
                l10n_util::GetStringUTF16(IDS_IPFS_SERVICE_LAUNCH_ERROR));
}

void IPFSOnboardingPage::GetConnectedPeers() {
  ipfs_service_->GetConnectedPeers(base::NullCallback());
}

bool IPFSOnboardingPage::IsLocalNodeMode() {
  auto* context = web_contents()->GetBrowserContext();
  auto* prefs = user_prefs::UserPrefs::Get(context);
  return (prefs->GetInteger(kIPFSResolveMethod) ==
          static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
}

void IPFSOnboardingPage::OnIpfsLaunched(bool result, int64_t pid) {
  if (!result) {
    ReportDaemonStopped();
    return;
  }

  if (!IsLocalNodeMode())
    return;
  RespondToPage(LOCAL_NODE_LAUNCHED, base::string16());
  GetConnectedPeers();
}

void IPFSOnboardingPage::Proceed() {
  service_observer_.Reset();
  controller()->OpenUrlInCurrentTab(request_url());
}

void IPFSOnboardingPage::RespondToPage(IPFSOnboardingResponse value,
                                       const base::string16& text) {
  auto* main_frame = web_contents()->GetMainFrame();
  DCHECK(main_frame);

  base::string16 script(base::UTF8ToUTF16(kResponseScript));
  base::ReplaceSubstringsAfterOffset(&script, 0, base::UTF8ToUTF16("{value}"),
                                     base::NumberToString16(value));
  base::ReplaceSubstringsAfterOffset(&script, 0, base::UTF8ToUTF16("{text}"),
                                     text);
  main_frame->ExecuteJavaScriptInIsolatedWorld(script, {},
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
      controller()->OpenUrlInNewForegroundTab(GURL(kIPFSLearnMoreURL));
      break;
    case IPFSOnboardingCommandId::OPEN_SETTINGS:
      controller()->OpenUrlInNewForegroundTab(GURL(kBraveSettingsURL));
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
      "localNodeButton",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_LOCAL_NODE_BUTTON));
  load_time_data->SetString(
      "publicGatewayButton",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_PUBLIC_GATEWAY_BUTTON));
  load_time_data->SetString(
      "learnMore", l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_LEARN_MORE));
  load_time_data->SetString(
      "localNodeText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_LOCAL_NODE_TEXT));
  load_time_data->SetString(
      "publicGatewayText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_PUBLIC_GATEWAY_TEXT));
  load_time_data->SetString(
      "footerText", l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_FOOTER_TEXT));
  load_time_data->SetString(
      "settings", l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_SETTINGS));
  load_time_data->SetString(
      "retryText", l10n_util::GetStringUTF16(IDS_IPFS_SERVICE_LAUNCH_RETRY));
  load_time_data->SetString(
      "installationText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_INSTALLATION_STATUS));
  load_time_data->SetString(
      "watingPeersText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_WAITING_PEERS_STATUS));
  load_time_data->SetString(
      "retryLimitPeersText",
      l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_PEERS_LIMIT_ERROR));
  load_time_data->SetString(
      "tryAgainText", l10n_util::GetStringUTF16(IDS_IPFS_ONBOARDING_TRY_AGAIN));

  load_time_data->SetString(
      "braveTheme", GetThemeType(ui::NativeTheme::GetInstanceForNativeUi()));
}

int IPFSOnboardingPage::GetHTMLTemplateId() {
  return IDR_IPFS_INTERSTITIAL_ONBOARDING_HTML;
}

security_interstitials::SecurityInterstitialPage::TypeID
IPFSOnboardingPage::GetTypeForTesting() {
  return IPFSOnboardingPage::kTypeForTesting;
}

std::string IPFSOnboardingPage::GetThemeType(ui::NativeTheme* theme) const {
  return theme->ShouldUseDarkColors() ? "dark" : "light";
}

void IPFSOnboardingPage::OnNativeThemeUpdated(ui::NativeTheme* observed_theme) {
  RespondToPage(THEME_CHANGED, base::UTF8ToUTF16(GetThemeType(observed_theme)));
}

}  // namespace ipfs
