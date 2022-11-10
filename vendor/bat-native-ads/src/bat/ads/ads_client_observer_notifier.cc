/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads_client_observer_notifier.h"

#include <utility>

namespace ads {

AdsClientObserverNotifier::AdsClientObserverNotifier() = default;

AdsClientObserverNotifier::~AdsClientObserverNotifier() = default;

void AdsClientObserverNotifier::AddBatAdsClientObserver(
    mojo::PendingRemote<bat_ads::mojom::BatAdsClientObserver> observer) {
  observers_.Add(std::move(observer));
}

void AdsClientObserverNotifier::NotifyLocaleDidChange(
    const std::string& locale) const {
  for (const auto& observer : observers_) {
    observer->OnLocaleDidChange(locale);
  }
}

void AdsClientObserverNotifier::NotifyPrefDidChange(
    const std::string& path) const {
  for (const auto& observer : observers_) {
    observer->OnPrefDidChange(path);
  }
}

void AdsClientObserverNotifier::NotifyDidUpdateResourceComponent(
    const std::string& id) const {
  for (const auto& observer : observers_) {
    observer->OnDidUpdateResourceComponent(id);
  }
}

void AdsClientObserverNotifier::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) const {
  for (const auto& observer : observers_) {
    observer->OnTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientObserverNotifier::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) const {
  for (const auto& observer : observers_) {
    observer->OnTabHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientObserverNotifier::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) const {
  for (const auto& observer : observers_) {
    observer->OnTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientObserverNotifier::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) const {
  for (const auto& observer : observers_) {
    observer->OnTabDidStopPlayingMedia(tab_id);
  }
}

void AdsClientObserverNotifier::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_visible,
    const bool is_incognito) const {
  for (const auto& observer : observers_) {
    observer->OnTabDidChange(tab_id, redirect_chain, is_visible, is_incognito);
  }
}

void AdsClientObserverNotifier::NotifyDidCloseTab(const int32_t tab_id) const {
  for (const auto& observer : observers_) {
    observer->OnDidCloseTab(tab_id);
  }
}

void AdsClientObserverNotifier::NotifyUserDidBecomeIdle() const {
  for (const auto& observer : observers_) {
    observer->OnUserDidBecomeIdle();
  }
}

void AdsClientObserverNotifier::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) const {
  for (const auto& observer : observers_) {
    observer->OnUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientObserverNotifier::NotifyBrowserDidEnterForeground() const {
  for (const auto& observer : observers_) {
    observer->OnBrowserDidEnterForeground();
  }
}

void AdsClientObserverNotifier::NotifyBrowserDidEnterBackground() const {
  for (const auto& observer : observers_) {
    observer->OnBrowserDidEnterBackground();
  }
}

void AdsClientObserverNotifier::NotifyBrowserDidBecomeActive() const {
  for (const auto& observer : observers_) {
    observer->OnBrowserDidBecomeActive();
  }
}

void AdsClientObserverNotifier::NotifyBrowserDidResignActive() const {
  for (const auto& observer : observers_) {
    observer->OnBrowserDidResignActive();
  }
}

void AdsClientObserverNotifier::NotifyRewardsWalletIsReady(
    const std::string& payment_id,
    const std::string& recovery_seed) const {
  for (const auto& observer : observers_) {
    observer->OnRewardsWalletIsReady(payment_id, recovery_seed);
  }
}

void AdsClientObserverNotifier::NotifyRewardsWalletDidChange(
    const std::string& payment_id,
    const std::string& recovery_seed) const {
  for (const auto& observer : observers_) {
    observer->OnRewardsWalletDidChange(payment_id, recovery_seed);
  }
}

}  // namespace ads
