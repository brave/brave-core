/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// based on //chrome/browser/ui/webui/browser_command/browser_command_handler.cc

#include "brave/browser/ui/webui/brave_browser_command/brave_browser_command_handler.h"

#include "base/containers/contains.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_education/education_urls.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#endif

namespace {

bool CanShowWalletOnboarding(Profile* profile) {
  return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
             profile) != nullptr;
}

bool CanShowRewardsOnboarding(Profile* profile) {
  return brave_rewards::RewardsServiceFactory::GetForProfile(profile) !=
         nullptr;
}

bool CanShowVPNBubble(Profile* profile) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  return brave_vpn::IsAllowedForContext(profile);
#else
  return false;
#endif
}

bool CanShowAIChat(Profile* profile) {
  return ai_chat::AIChatServiceFactory::GetForBrowserContext(profile) !=
         nullptr;
}

}  // namespace

BraveBrowserCommandHandler::BraveBrowserCommandHandler(
    mojo::PendingReceiver<
        brave_browser_command::mojom::BraveBrowserCommandHandler>
        pending_page_handler,
    Profile* profile,
    std::vector<brave_browser_command::mojom::Command> supported_commands,
    std::unique_ptr<Delegate> delegate)
    : profile_(profile),
      supported_commands_(supported_commands),
      delegate_(std::move(delegate)),
      page_handler_(this, std::move(pending_page_handler)) {}

BraveBrowserCommandHandler::~BraveBrowserCommandHandler() = default;

void BraveBrowserCommandHandler::CanExecuteCommand(
    brave_browser_command::mojom::Command command_id,
    CanExecuteCommandCallback callback) {
  if (!base::Contains(supported_commands_, command_id)) {
    std::move(callback).Run(false);
    return;
  }

  bool can_execute = false;
  switch (command_id) {
    case brave_browser_command::mojom::Command::kOpenWalletOnboarding:
      can_execute = CanShowWalletOnboarding(profile_);
      break;
    case brave_browser_command::mojom::Command::kOpenRewardsOnboarding:
      can_execute = CanShowRewardsOnboarding(profile_);
      break;
    case brave_browser_command::mojom::Command::kOpenVPNOnboarding:
      can_execute = CanShowVPNBubble(profile_);
      break;
    case brave_browser_command::mojom::Command::kOpenAIChat:
      can_execute = CanShowAIChat(profile_);
      break;
  }
  std::move(callback).Run(can_execute);
}

void BraveBrowserCommandHandler::ExecuteCommand(
    brave_browser_command::mojom::Command command_id,
    ExecuteCommandCallback callback) {
  if (!base::Contains(supported_commands_, command_id)) {
    std::move(callback).Run(false);
    return;
  }

  switch (command_id) {
    case brave_browser_command::mojom::Command::kOpenWalletOnboarding:
      delegate_->OpenURL(GURL(kBraveUIWalletURL),
                         WindowOpenDisposition::NEW_FOREGROUND_TAB);
      break;
    case brave_browser_command::mojom::Command::kOpenRewardsOnboarding:
      delegate_->OpenRewardsPanel();
      break;
    case brave_browser_command::mojom::Command::kOpenVPNOnboarding:
      delegate_->OpenVPNPanel();
      break;
    case brave_browser_command::mojom::Command::kOpenAIChat:
      delegate_->OpenAIChat();
      break;
  }

  std::move(callback).Run(true);
}
