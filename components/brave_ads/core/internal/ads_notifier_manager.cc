/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_observer.h"

namespace brave_ads {

AdsNotifierManager::AdsNotifierManager() = default;

AdsNotifierManager::~AdsNotifierManager() = default;

// static
AdsNotifierManager& AdsNotifierManager::GetInstance() {
  return GlobalState::GetInstance()->GetAdsNotifierManager();
}

void AdsNotifierManager::AddObserver(std::unique_ptr<AdsObserver> observer) {
  observers_.push_back(std::move(observer));
}

void AdsNotifierManager::NotifyAdRewardsDidChange() const {
  for (const auto& ads_observer : observers_) {
    ads_observer->OnAdRewardsDidChange();
  }
}

void AdsNotifierManager::NotifyBrowserUpgradeRequiredToServeAds() const {
  for (const auto& ads_observer : observers_) {
    ads_observer->OnBrowserUpgradeRequiredToServeAds();
  }
}

void AdsNotifierManager::NotifyIneligibleWalletToServeAds() const {
  for (const auto& ads_observer : observers_) {
    ads_observer->OnIneligibleWalletToServeAds();
  }
}

void AdsNotifierManager::NotifyRemindUser(
    mojom::ReminderType mojom_reminder_type) const {
  for (const auto& ads_observer : observers_) {
    ads_observer->OnRemindUser(mojom_reminder_type);
  }
}

}  // namespace brave_ads
