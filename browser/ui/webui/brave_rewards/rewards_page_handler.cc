/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_page_handler.h"

#include <utility>

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

namespace brave_rewards {

RewardsPageHandler::RewardsPageHandler(
    mojo::PendingRemote<mojom::RewardsPage> page,
    mojo::PendingReceiver<mojom::RewardsPageHandler> receiver,
    std::unique_ptr<BubbleDelegate> bubble_delegate,
    RewardsService* rewards_service)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      bubble_delegate_(std::move(bubble_delegate)),
      rewards_service_(rewards_service) {
  CHECK(rewards_service_);
  rewards_observation_.Observe(rewards_service_);
}

RewardsPageHandler::~RewardsPageHandler() = default;

void RewardsPageHandler::OnPageReady() {
  if (bubble_delegate_) {
    bubble_delegate_->ShowUI();
  }
}

void RewardsPageHandler::OpenTab(const std::string& url) {
  if (bubble_delegate_) {
    bubble_delegate_->OpenTab(url);
  }
}

void RewardsPageHandler::GetAvailableCountries(
    GetAvailableCountriesCallback callback) {
  auto get_countries_callback = [](decltype(callback) callback,
                                   mojom::AvailableCountryInfoPtr info,
                                   std::vector<std::string> country_codes) {
    info->country_codes = std::move(country_codes);
    std::move(callback).Run(std::move(info));
  };

  auto info = mojom::AvailableCountryInfo::New();
  info->default_country_code = rewards_service_->GetCountryCode();

  rewards_service_->GetAvailableCountries(base::BindOnce(
      get_countries_callback, std::move(callback), std::move(info)));
}

void RewardsPageHandler::GetRewardsPaymentId(
    GetRewardsPaymentIdCallback callback) {
  auto get_wallet_callback = [](decltype(callback) callback,
                                mojom::RewardsWalletPtr rewards_wallet) {
    std::string payment_id;
    if (rewards_wallet) {
      payment_id = rewards_wallet->payment_id;
    }
    std::move(callback).Run(std::move(payment_id));
  };

  rewards_service_->GetRewardsWallet(
      base::BindOnce(get_wallet_callback, std::move(callback)));
}

void RewardsPageHandler::EnableRewards(const std::string& country_code,
                                       EnableRewardsCallback callback) {
  rewards_service_->CreateRewardsWallet(country_code, std::move(callback));
}

void RewardsPageHandler::ResetRewards(ResetRewardsCallback callback) {
  rewards_service_->CompleteReset(std::move(callback));
}

void RewardsPageHandler::OnRewardsInitialized(RewardsService* rewards_service) {
  page_->OnRewardsStateUpdated();
}

void RewardsPageHandler::OnRewardsWalletCreated() {
  page_->OnRewardsStateUpdated();
}

void RewardsPageHandler::OnCompleteReset(bool success) {
  page_->OnRewardsStateUpdated();
}

}  // namespace brave_rewards
