/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/education_page_handler.h"

#include "base/containers/fixed_flat_set.h"
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

namespace brave_education {

namespace {

bool IsCommandSupportedForPageType(mojom::Command command,
                                   EducationPageType page_type) {
  switch (page_type) {
    case EducationPageType::kGettingStarted: {
      static constexpr auto kCommands = base::MakeFixedFlatSet<mojom::Command>(
          {mojom::Command::kOpenRewardsOnboarding,
           mojom::Command::kOpenWalletOnboarding,
           mojom::Command::kOpenVPNOnboarding, mojom::Command::kOpenAIChat});
      return kCommands.contains(command);
    }
  }
  NOTREACHED();
}

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

EducationPageHandler::EducationPageHandler(
    mojo::PendingReceiver<mojom::EducationPageHandler> receiver,
    Profile* profile,
    EducationPageType page_type,
    std::unique_ptr<Delegate> delegate)
    : receiver_(this, std::move(receiver)),
      profile_(profile),
      page_type_(page_type),
      delegate_(std::move(delegate)) {}

EducationPageHandler::~EducationPageHandler() = default;

void EducationPageHandler::GetServerUrl(GetServerUrlCallback callback) {
  std::move(callback).Run(GetEducationPageServerURL(page_type_).spec());
}

void EducationPageHandler::ExecuteCommand(mojom::Command command,
                                          ExecuteCommandCallback callback) {
  if (!CanExecute(command)) {
    std::move(callback).Run(false);
    return;
  }

  switch (command) {
    case mojom::Command::kOpenWalletOnboarding:
      delegate_->OpenURL(GURL(kBraveUIWalletURL),
                         WindowOpenDisposition::NEW_FOREGROUND_TAB);
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
  if (!IsCommandSupportedForPageType(command, page_type_)) {
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
