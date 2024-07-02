/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/browser_command/brave_browser_command_handler.h"

#include <optional>
#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#endif

namespace {

using browser_command::mojom::Command;
using browser_command::mojom::CommandHandler;

// A `WebContentsObserver` that listens for the first document load event in
// the primary frame for a tab, and then attempts to open the shields bubble for
// that window. It is indirectly owned by the WebContents and deletes itself
// when the first load event occurs (or the web contents is destoryed).
class ShieldsBubbleOpener : public content::WebContentsObserver {
 public:
  static void MaybeCreateForWebContents(content::WebContents* web_contents) {
    if (!web_contents) {
      return;
    }
    new ShieldsBubbleOpener(web_contents);
  }

  void WebContentsDestroyed() override { delete this; }

  void DocumentOnLoadCompletedInPrimaryMainFrame() override {
    brave::ShowShieldsBubble(chrome::FindBrowserWithTab(web_contents()));
    delete this;
  }

 private:
  using WebContentsObserver::WebContentsObserver;
  ~ShieldsBubbleOpener() override = default;
};

Browser* GetBrowser(Profile* profile) {
  return chrome::FindBrowserWithProfile(profile);
}

void OpenURL(Profile* profile,
             const GURL& url,
             WindowOpenDisposition disposition) {
  NavigateParams params(profile, url, ui::PAGE_TRANSITION_LINK);
  params.disposition = disposition;
  Navigate(&params);
}

void OpenShieldsBubble(Profile* profile) {
  NavigateParams params(profile, GURL("https://brave.com"),
                        ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  if (auto navigation_handle = Navigate(&params)) {
    ShieldsBubbleOpener::MaybeCreateForWebContents(
        navigation_handle->GetWebContents());
  }
}

bool CanShowWalletOnboarding(Profile* profile) {
  CHECK(profile);
  return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
}

bool CanShowRewardsOnboarding(Profile* profile) {
  CHECK(profile);
  return brave_rewards::RewardsServiceFactory::GetForProfile(profile);
}

bool OpenRewardsOnboarding(Profile* profile) {
  CHECK(profile);
  auto* panel_coordinator =
      brave_rewards::RewardsPanelCoordinator::FromBrowser(GetBrowser(profile));
  return panel_coordinator && panel_coordinator->OpenRewardsPanel();
}

}  // namespace

BraveBrowserCommandHandler::BraveBrowserCommandHandler(
    mojo::PendingReceiver<CommandHandler> pending_command_handler,
    Profile* profile,
    std::vector<Command> supported_commands)
    : BrowserCommandHandler(std::move(pending_command_handler),
                            profile,
                            std::move(supported_commands)),
      profile_(profile) {
  CHECK(profile_);
}

BraveBrowserCommandHandler::~BraveBrowserCommandHandler() = default;

void BraveBrowserCommandHandler::CanExecuteCommand(
    Command command,
    CanExecuteCommandCallback callback) {
  if (auto can_execute_result = CanExecute(command)) {
    std::move(callback).Run(can_execute_result.value());
  } else {
    BrowserCommandHandler::CanExecuteCommand(command, std::move(callback));
  }
}

void BraveBrowserCommandHandler::ExecuteCommandWithDisposition(
    int id,
    WindowOpenDisposition disposition) {
  Command command = static_cast<Command>(id);
  if (!CanExecute(command)) {
    return;
  }

  switch (static_cast<Command>(id)) {
    case Command::kOpenWalletOnboarding:
      OpenURL(profile_, GURL(kBraveUIWalletURL), disposition);
      break;
    case Command::kOpenWeb3Settings:
      OpenURL(profile_, GURL(chrome::GetSettingsUrl("web3")), disposition);
      break;
    case Command::kOpenRewardsOnboarding:
      OpenRewardsOnboarding(profile_);
      break;
    case Command::kOpenContentFilterSettings:
      OpenURL(profile_, GURL(chrome::GetSettingsUrl("shields/filters")),
              disposition);
      break;
    case Command::kOpenShieldsSettings:
      OpenURL(profile_, GURL(chrome::GetSettingsUrl("shields")), disposition);
      break;
    case Command::kOpenShieldsPanel:
      OpenShieldsBubble(profile_);
      break;
    case Command::kOpenPrivacySettings:
      OpenURL(profile_, GURL(chrome::GetSettingsUrl("privacy")), disposition);
      break;
    case Command::kOpenVPNOnboarding:
#if BUILDFLAG(ENABLE_BRAVE_VPN)
      brave::ShowBraveVPNBubble(GetBrowser(profile_));
#endif
      break;
    default:
      BrowserCommandHandler::ExecuteCommandWithDisposition(id, disposition);
      break;
  }
}

std::optional<bool> BraveBrowserCommandHandler::CanExecute(Command command) {
  if (!GetCommandUpdater()->SupportsCommand(static_cast<int>(command))) {
    return false;
  }
  switch (command) {
    case Command::kOpenWalletOnboarding:
      return CanShowWalletOnboarding(profile_);
    case Command::kOpenWeb3Settings:
      return true;
    case Command::kOpenRewardsOnboarding:
      return CanShowRewardsOnboarding(profile_);
    case Command::kOpenContentFilterSettings:
      return true;
    case Command::kOpenShieldsSettings:
      return true;
    case Command::kOpenShieldsPanel:
      return true;
    case Command::kOpenPrivacySettings:
      return true;
    case Command::kOpenVPNOnboarding:
#if BUILDFLAG(ENABLE_BRAVE_VPN)
      return brave_vpn::IsAllowedForContext(profile_);
#else
      return false;
#endif
    default:
      return std::nullopt;
  }
}
