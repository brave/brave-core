/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_page_handler.h"

#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/functional/callback.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_rewards {

namespace {

static constexpr auto kPluralStrings =
    base::MakeFixedFlatMap<std::string_view, int>(
        {{"unconnectedAdsViewedText",
          IDS_REWARDS_UNCONNECTED_ADS_VIEWED_TEXT}});

}  // namespace

// Listens for updates to browser data displayed on the Rewards page and
// executes a callback when updates occur.
class RewardsPageHandler::UpdateObserver
    : public RewardsServiceObserver,
      public bat_ads::mojom::BatAdsObserver {
 public:
  UpdateObserver(RewardsService* rewards_service,
                 brave_ads::AdsService* ads_service,
                 base::RepeatingCallback<void(UpdateSource)> update_callback)
      : update_callback_(std::move(update_callback)) {
    rewards_observation_.Observe(rewards_service);
    ads_service->AddBatAdsObserver(
        ads_observer_receiver_.BindNewPipeAndPassRemote());
  }

  // RewardsServiceObserver:
  void OnRewardsInitialized(RewardsService*) override {
    OnUpdate(UpdateSource::kRewards);
  }

  void OnRewardsWalletCreated() override { OnUpdate(UpdateSource::kRewards); }

  void OnCompleteReset(bool success) override {
    OnUpdate(UpdateSource::kRewards);
  }

  void OnExternalWalletConnected() override {
    OnUpdate(UpdateSource::kRewards);
  }

  void OnExternalWalletLoggedOut() override {
    OnUpdate(UpdateSource::kRewards);
  }

  void OnExternalWalletReconnected() override {
    OnUpdate(UpdateSource::kRewards);
  }

  void OnExternalWalletDisconnected() override {
    OnUpdate(UpdateSource::kRewards);
  }

  // bat_ads::mojom::BatAdsObserver:
  void OnAdRewardsDidChange() override { OnUpdate(UpdateSource::kAds); }
  void OnBrowserUpgradeRequiredToServeAds() override {}
  void OnIneligibleRewardsWalletToServeAds() override {}
  void OnRemindUser(brave_ads::mojom::ReminderType type) override {}

 private:
  void OnUpdate(UpdateSource update_source) {
    update_callback_.Run(update_source);
  }

  base::ScopedObservation<RewardsService, RewardsServiceObserver>
      rewards_observation_{this};
  mojo::Receiver<bat_ads::mojom::BatAdsObserver> ads_observer_receiver_{this};
  base::RepeatingCallback<void(UpdateSource)> update_callback_;
};

RewardsPageHandler::RewardsPageHandler(
    mojo::PendingRemote<mojom::RewardsPage> page,
    mojo::PendingReceiver<mojom::RewardsPageHandler> receiver,
    std::unique_ptr<BubbleDelegate> bubble_delegate,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      bubble_delegate_(std::move(bubble_delegate)),
      rewards_service_(RewardsServiceFactory::GetForProfile(profile)),
      ads_service_(brave_ads::AdsServiceFactory::GetForProfile(profile)) {
  CHECK(rewards_service_);
  CHECK(ads_service_);

  // Unretained because `update_observer_` is owned by `this`.
  update_observer_ = std::make_unique<UpdateObserver>(
      rewards_service_, ads_service_,
      base::BindRepeating(&RewardsPageHandler::OnUpdate,
                          base::Unretained(this)));
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

void RewardsPageHandler::GetPluralString(const std::string& key,
                                         int32_t count,
                                         GetPluralStringCallback callback) {
  auto iter = kPluralStrings.find(key);
  CHECK(iter != kPluralStrings.end());
  std::move(callback).Run(l10n_util::GetPluralStringFUTF8(iter->second, count));
}

void RewardsPageHandler::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  rewards_service_->GetRewardsParameters(std::move(callback));
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

void RewardsPageHandler::GetCountryCode(GetCountryCodeCallback callback) {
  std::move(callback).Run(rewards_service_->GetCountryCode());
}

void RewardsPageHandler::GetExternalWallet(GetExternalWalletCallback callback) {
  rewards_service_->GetExternalWallet(std::move(callback));
}

void RewardsPageHandler::GetExternalWalletProviders(
    GetExternalWalletProvidersCallback callback) {
  std::move(callback).Run(rewards_service_->GetExternalWalletProviders());
}

void RewardsPageHandler::GetAdsStatement(GetAdsStatementCallback callback) {
  ads_service_->GetStatementOfAccounts(std::move(callback));
}

void RewardsPageHandler::EnableRewards(const std::string& country_code,
                                       EnableRewardsCallback callback) {
  rewards_service_->CreateRewardsWallet(country_code, std::move(callback));
}

void RewardsPageHandler::BeginExternalWalletLogin(
    const std::string& provider,
    BeginExternalWalletLoginCallback callback) {
  rewards_service_->BeginExternalWalletLogin(provider, std::move(callback));
}

void RewardsPageHandler::ConnectExternalWallet(
    const std::string& provider,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  rewards_service_->ConnectExternalWallet(provider, args, std::move(callback));
}

void RewardsPageHandler::ResetRewards(ResetRewardsCallback callback) {
  rewards_service_->CompleteReset(std::move(callback));
}

void RewardsPageHandler::OnUpdate(UpdateSource update_source) {
  page_->OnRewardsStateUpdated();
}

}  // namespace brave_rewards
