/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ads_client_notifier_manager.h"

#include <utility>

namespace brave_ads {

AdsClientNotifierManager::AdsClientNotifierManager() = default;

AdsClientNotifierManager::~AdsClientNotifierManager() = default;

void AdsClientNotifierManager::AddObserver(AdsClientObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdsClientNotifierManager::RemoveObserver(AdsClientObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdsClientNotifierManager::NotifyLocaleDidChange(
    const std::string& locale) {
  for (auto& observer : observers_) {
    observer.OnLocaleDidChange(locale);
  }
}

void AdsClientNotifierManager::NotifyPrefDidChange(const std::string& path) {
  for (auto& observer : observers_) {
    observer.OnPrefDidChange(path);
  }
}

void AdsClientNotifierManager::NotifyDidUpdateResourceComponent(
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDidUpdateResourceComponent(id);
  }
}

void AdsClientNotifierManager::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  for (auto& observer : observers_) {
    observer.OnTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientNotifierManager::NotifyTabHtmlContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  for (auto& observer : observers_) {
    observer.OnTabHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientNotifierManager::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientNotifierManager::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnTabDidStopPlayingMedia(tab_id);
  }
}

void AdsClientNotifierManager::NotifyTabDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    bool is_visible,
    bool is_incognito) {
  for (auto& observer : observers_) {
    observer.OnTabDidChange(tab_id, redirect_chain, is_visible, is_incognito);
  }
}

void AdsClientNotifierManager::NotifyDidCloseTab(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnDidCloseTab(tab_id);
  }
}

void AdsClientNotifierManager::NotifyUserDidBecomeIdle() {
  for (auto& observer : observers_) {
    observer.OnUserDidBecomeIdle();
  }
}

void AdsClientNotifierManager::NotifyUserDidBecomeActive(
    base::TimeDelta idle_time,
    bool screen_was_locked) {
  for (auto& observer : observers_) {
    observer.OnUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientNotifierManager::NotifyBrowserDidEnterForeground() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidEnterForeground();
  }
}

void AdsClientNotifierManager::NotifyBrowserDidEnterBackground() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidEnterBackground();
  }
}

void AdsClientNotifierManager::NotifyBrowserDidBecomeActive() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidBecomeActive();
  }
}

void AdsClientNotifierManager::NotifyBrowserDidResignActive() {
  for (auto& observer : observers_) {
    observer.OnBrowserDidResignActive();
  }
}

}  // namespace brave_ads
