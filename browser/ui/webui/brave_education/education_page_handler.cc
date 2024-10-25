/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/education_page_handler.h"

#include <array>

#include "base/containers/contains.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_education/common/education_content_urls.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "ui/base/window_open_disposition_utils.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#endif

namespace brave_education {

namespace {

bool IsCommandSupportedForContentType(mojom::Command command,
                                      EducationContentType content_type) {
  switch (content_type) {
    case EducationContentType::kGettingStarted: {
      constexpr auto kCommands = std::to_array(
          {mojom::Command::kOpenRewardsOnboarding,
           mojom::Command::kOpenWalletOnboarding,
           mojom::Command::kOpenVPNOnboarding, mojom::Command::kOpenAIChat});
      return base::Contains(kCommands, command);
    }
  }
}

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

class BrowserDelegate : public EducationPageHandler::Delegate {
 public:
  explicit BrowserDelegate(Profile* profile) : profile_(profile) {}
  ~BrowserDelegate() override = default;

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

  void OpenAIChat() override {
    chrome::ExecuteCommand(GetBrowser(), IDC_TOGGLE_AI_CHAT);
  }

 private:
  Browser* GetBrowser() { return chrome::FindBrowserWithProfile(profile_); }

  raw_ptr<Profile> profile_;
};

}  // namespace

EducationPageHandler::EducationPageHandler(
    mojo::PendingReceiver<mojom::EducationPageHandler> receiver,
    Profile* profile,
    std::optional<EducationContentType> content_type)
    : receiver_(this, std::move(receiver)),
      profile_(profile),
      content_type_(content_type),
      delegate_(std::make_unique<BrowserDelegate>(profile)) {}

EducationPageHandler::~EducationPageHandler() = default;

void EducationPageHandler::GetServerUrl(GetServerUrlCallback callback) {
  if (!content_type_) {
    std::move(callback).Run("");
    return;
  }
  auto server_url = GetEducationContentServerURL(*content_type_);
  std::move(callback).Run(server_url.spec());
}

void EducationPageHandler::ExecuteCommand(mojom::Command command,
                                          mojom::ClickInfoPtr click_info,
                                          ExecuteCommandCallback callback) {
  if (!CanExecute(command)) {
    std::move(callback).Run(false);
    return;
  }

  auto disposition = ui::DispositionFromClick(
      click_info->middle_button, click_info->alt_key, click_info->ctrl_key,
      click_info->meta_key, click_info->shift_key);

  switch (command) {
    case mojom::Command::kOpenWalletOnboarding:
      delegate_->OpenURL(GURL(kBraveUIWalletURL), disposition);
      break;
    case mojom::Command::kOpenRewardsOnboarding:
      delegate_->OpenRewardsPanel();
      break;
    case mojom::Command::kOpenVPNOnboarding:
      delegate_->OpenVPNPanel();
      break;
    case mojom::Command::kOpenAIChat:
      delegate_->OpenAIChat();
      break;
  }

  std::move(callback).Run(true);
}

bool EducationPageHandler::CanExecute(mojom::Command command) {
  if (!content_type_ ||
      !IsCommandSupportedForContentType(command, *content_type_)) {
    return false;
  }
  switch (command) {
    case mojom::Command::kOpenWalletOnboarding:
      return CanShowWalletOnboarding(profile_);
    case mojom::Command::kOpenRewardsOnboarding:
      return CanShowRewardsOnboarding(profile_);
    case mojom::Command::kOpenVPNOnboarding:
      return CanShowVPNBubble(profile_);
    case mojom::Command::kOpenAIChat:
      return CanShowAIChat(profile_);
  }
}

}  // namespace brave_education
