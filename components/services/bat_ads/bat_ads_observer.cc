/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_observer.h"

#include <utility>
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace bat_ads {

BatAdsObserver::BatAdsObserver(mojo::PendingRemote<mojom::BatAdsObserver>
                                   bat_ads_observer_pending_remote) {
  bat_ads_observer_remote_.Bind(std::move(bat_ads_observer_pending_remote));
}

BatAdsObserver::~BatAdsObserver() = default;

void BatAdsObserver::OnAdRewardsDidChange() {
  bat_ads_observer_remote_->OnAdRewardsDidChange();
}

void BatAdsObserver::OnBrowserUpgradeRequiredToServeAds() {
  bat_ads_observer_remote_->OnBrowserUpgradeRequiredToServeAds();
}

void BatAdsObserver::OnIneligibleWalletToServeAds() {
  bat_ads_observer_remote_->OnIneligibleWalletToServeAds();
}

void BatAdsObserver::OnRemindUser(
    const brave_ads::mojom::ReminderType mojom_reminder_type) {
  bat_ads_observer_remote_->OnRemindUser(mojom_reminder_type);
}

}  // namespace bat_ads
