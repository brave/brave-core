/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/browser_command/brave_browser_command_handler.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#endif

namespace {

using browser_command::mojom::Command;
using browser_command::mojom::CommandHandler;

bool CanShowWalletOnboarding(Profile* profile) {
  return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
}

bool CanShowRewardsOnboarding(Profile* profile) {
  return brave_rewards::RewardsServiceFactory::GetForProfile(profile);
}

bool CanShowVPNBubble(Profile* profile) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  return brave_vpn::IsAllowedForContext(profile);
#else
  return false;
#endif
}

bool CanShowAIChat(Profile* profile) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  return ai_chat::IsAllowedForContext(profile);
#else
  return false;
#endif
}

class Delegate : public BraveBrowserCommandHandler::BrowserDelegate {
 public:
  explicit Delegate(Profile* profile) : profile_(profile) {}
  ~Delegate() override = default;

  void OpenURL(const GURL& url, WindowOpenDisposition disposition) override {
    NavigateParams params(profile_, url, ui::PAGE_TRANSITION_LINK);
    params.disposition = disposition;
    Navigate(&params);
  }

  void OpenRewardsPanel() override {
    auto* panel_coordinator =
        brave_rewards::RewardsPanelCoordinator::FromBrowser(GetBrowser());
    if (panel_coordinator) {
      panel_coordinator->OpenRewardsPanel();
    }
  }

  void OpenVPNPanel() override {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    brave::ShowBraveVPNBubble(GetBrowser());
#endif
  }

  void ExecuteBrowserCommand(int command_id) override {
    chrome::ExecuteCommand(GetBrowser(), command_id);
  }

 private:
  Browser* GetBrowser() { return chrome::FindBrowserWithProfile(profile_); }

  raw_ptr<Profile> profile_;
};

}  // namespace

BraveBrowserCommandHandler::BraveBrowserCommandHandler(
    mojo::PendingReceiver<CommandHandler> pending_command_handler,
    Profile* profile,
    std::vector<Command> supported_commands)
    : BrowserCommandHandler(std::move(pending_command_handler),
                            profile,
                            std::move(supported_commands)),
      delegate_(std::make_unique<Delegate>(profile)),
      profile_(profile) {
  CHECK(profile_);
}

BraveBrowserCommandHandler::~BraveBrowserCommandHandler() = default;

void BraveBrowserCommandHandler::CanExecuteCommand(
    Command command,
    CanExecuteCommandCallback callback) {
  if (CanExecute(command)) {
    std::move(callback).Run(true);
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
      delegate_->OpenURL(GURL(kBraveUIWalletURL), disposition);
      break;
    case Command::kOpenRewardsOnboarding:
      delegate_->OpenRewardsPanel();
      break;
    case Command::kOpenVPNOnboarding:
      delegate_->OpenVPNPanel();
      break;
    case Command::kOpenAIChat:
      delegate_->ExecuteBrowserCommand(IDC_TOGGLE_AI_CHAT);
      break;
    default:
      break;
  }
}

bool BraveBrowserCommandHandler::CanExecute(Command command) {
  if (!GetCommandUpdater()->SupportsCommand(static_cast<int>(command))) {
    return false;
  }
  switch (command) {
    case Command::kOpenWalletOnboarding:
      return CanShowWalletOnboarding(profile_);
    case Command::kOpenRewardsOnboarding:
      return CanShowRewardsOnboarding(profile_);
    case Command::kOpenVPNOnboarding:
      return CanShowVPNBubble(profile_);
    case Command::kOpenAIChat:
      return CanShowAIChat(profile_);
    default:
      return false;
  }
}
