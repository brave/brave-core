/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads_client_observer_manager.h"

#include <utility>

namespace ads {

AdsClientObserverManager::AdsClientObserverManager() = default;

AdsClientObserverManager::~AdsClientObserverManager() = default;

void AdsClientObserverManager::AddObserver(ads::AdsClientObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdsClientObserverManager::RemoveObserver(
    ads::AdsClientObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdsClientObserverManager::Clear() {
  observers_.Clear();
}

void AdsClientObserverManager::NotifyLocaleDidChange(
    const std::string& locale) {
  for (auto& observer : observers_) {
    observer.OnLocaleDidChange(locale);
  }
}

void AdsClientObserverManager::NotifyPrefDidChange(const std::string& path) {
  for (auto& observer : observers_) {
    observer.OnPrefDidChange(path);
  }
}

void AdsClientObserverManager::NotifyDidUpdateResourceComponent(
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDidUpdateResourceComponent(id);
  }
}

void AdsClientObserverManager::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  for (auto& observer : observers_) {
    observer.OnTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientObserverManager::NotifyTabHtmlContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  for (auto& observer : observers_) {
    observer.OnTabTextContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientObserverManager::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientObserverManager::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnTabDidStopPlayingMedia(tab_id);
  }
}

void AdsClientObserverManager::NotifyTabDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    bool is_visible,
    bool is_incognito) {
  for (auto& observer : observers_) {
    observer.OnTabDidChange(tab_id, redirect_chain, is_visible, is_incognito);
  }
}

void AdsClientObserverManager::NotifyDidCloseTab(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnDidCloseTab(tab_id);
  }
}

void AdsClientObserverManager::NotifyUserDidBecomeIdle() {
  for (auto& observer : observers_) {
    observer.OnUserDidBecomeIdle();
  }
}

void AdsClientObserverManager::NotifyUserDidBecomeActive(
    base::TimeDelta idle_time,
    bool screen_was_locked) {
  for (auto& observer : observers_) {
    observer.OnUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientObserverManager::NotifyBrowserDidEnterForeground() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidEnterForeground();
  }
}

void AdsClientObserverManager::NotifyBrowserDidEnterBackground() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidEnterBackground();
  }
}

void AdsClientObserverManager::NotifyBrowserDidBecomeActive() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidBecomeActive();
  }
}

void AdsClientObserverManager::NotifyBrowserDidResignActive() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidResignActive();
  }
}

void AdsClientObserverManager::NotifyRewardsWalletIsReady(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  for (auto& observer : observers_) {
    observer.OnRewardsWalletIsReady(payment_id, recovery_seed);
  }
}

void AdsClientObserverManager::NotifyRewardsWalletDidChange(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  for (auto& observer : observers_) {
    observer.OnRewardsWalletDidChange(payment_id, recovery_seed);
  }
}

}  // namespace ads
