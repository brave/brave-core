/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/tip_panel_handler.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_rewards/tip_panel_coordinator.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "content/public/common/url_constants.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_rewards {

namespace {

TipPanelCoordinator* GetCoordinator(Profile* profile) {
  DCHECK(profile);
  if (auto* browser = chrome::FindLastActiveWithProfile(profile)) {
    return TipPanelCoordinator::FromBrowser(browser);
  }
  return nullptr;
}

std::string GetRequestedPublisherID(Profile* profile) {
  if (auto* coordinator = GetCoordinator(profile)) {
    return coordinator->publisher_id();
  }
  return "";
}

gfx::Size GetCurrentBrowserSize(Profile* profile) {
  if (auto* coordinator = GetCoordinator(profile)) {
    return coordinator->browser_size();
  }
  return gfx::Size();
}

}  // namespace

TipPanelHandler::TipPanelHandler(
    mojo::PendingRemote<mojom::TipPanel> banner,
    mojo::PendingReceiver<mojom::TipPanelHandler> receiver,
    base::WeakPtr<TopChromeWebUIController::Embedder> embedder,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      banner_(std::move(banner)),
      embedder_(embedder),
      profile_(profile),
      rewards_service_(RewardsServiceFactory::GetForProfile(profile)),
      publisher_id_(GetRequestedPublisherID(profile)) {
  DCHECK(rewards_service_);
}

TipPanelHandler::~TipPanelHandler() = default;

void TipPanelHandler::ShowUI() {
  if (embedder_) {
    embedder_->ShowUI();
  }
}

void TipPanelHandler::CloseUI() {
  if (embedder_) {
    embedder_->CloseUI();
  }
}

void TipPanelHandler::GetBrowserSize(GetBrowserSizeCallback callback) {
  gfx::Size size = GetCurrentBrowserSize(profile_);
  std::move(callback).Run(size.width(), size.height());
}

void TipPanelHandler::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  DCHECK(rewards_service_);
  rewards_service_->GetRewardsParameters(std::move(callback));
}

void TipPanelHandler::GetBalance(GetBalanceCallback callback) {
  DCHECK(rewards_service_);
  rewards_service_->FetchBalance(std::move(callback));
}

void TipPanelHandler::GetBanner(GetBannerCallback callback) {
  DCHECK(rewards_service_);

  if (publisher_id_.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }

  rewards_service_->GetPublisherBanner(publisher_id_, std::move(callback));
}

void TipPanelHandler::GetMonthlyContributionSet(
    GetMonthlyContributionSetCallback callback) {
  DCHECK(rewards_service_);

  auto fn = [](GetMonthlyContributionSetCallback callback,
               std::string publisher_id,
               std::vector<mojom::PublisherInfoPtr> publishers) {
    for (auto& info : publishers) {
      if (info->id == publisher_id) {
        std::move(callback).Run(info->weight > 0);
        return;
      }
    }
    std::move(callback).Run(false);
  };

  rewards_service_->GetRecurringTips(
      base::BindOnce(fn, std::move(callback), publisher_id_));
}

void TipPanelHandler::GetExternalWallet(GetExternalWalletCallback callback) {
  DCHECK(rewards_service_);
  rewards_service_->GetExternalWallet(std::move(callback));
}

void TipPanelHandler::SendContribution(double amount,
                                       bool set_monthly,
                                       SendContributionCallback callback) {
  DCHECK(rewards_service_);

  if (publisher_id_.empty()) {
    std::move(callback).Run(false);
    return;
  }

  rewards_service_->SendContribution(publisher_id_, amount, set_monthly,
                                     std::move(callback));
}

void TipPanelHandler::OpenTab(const std::string& url) {
  GURL target_url(url);
  if (!target_url.is_valid()) {
    return;
  }
  if (auto* browser = chrome::FindLastActiveWithProfile(profile_)) {
    chrome::AddTabAt(browser, target_url, -1, true);
  }
}

}  // namespace brave_rewards
