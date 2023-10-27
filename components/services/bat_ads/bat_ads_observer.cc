/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_observer.h"

#include <utility>
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace bat_ads {

BatAdsObserver::BatAdsObserver() = default;

BatAdsObserver::BatAdsObserver(
    mojo::PendingRemote<mojom::BatAdsObserver> observer) {
  observer_.Bind(std::move(observer));
}

BatAdsObserver::~BatAdsObserver() = default;

void BatAdsObserver::OnAdRewardsDidChange() {
  observer_->OnAdRewardsDidChange();
}

void BatAdsObserver::OnBrowserUpgradeRequiredToServeAds() {
  observer_->OnBrowserUpgradeRequiredToServeAds();
}

void BatAdsObserver::OnIneligibleRewardsWalletToServeAds() {
  observer_->OnIneligibleRewardsWalletToServeAds();
}

void BatAdsObserver::OnRemindUser(const brave_ads::mojom::ReminderType type) {
  observer_->OnRemindUser(type);
}

}  // namespace bat_ads
