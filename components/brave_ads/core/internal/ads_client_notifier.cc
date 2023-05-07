/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ads_client_notifier.h"

#include "url/gurl.h"

namespace brave_ads {

AdsClientNotifier::AdsClientNotifier() = default;

AdsClientNotifier::~AdsClientNotifier() = default;

void AdsClientNotifier::AddObserver(AdsClientNotifierObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void AdsClientNotifier::RemoveObserver(AdsClientNotifierObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdsClientNotifier::NotifyLocaleDidChange(const std::string& locale) const {
  for (auto& observer : observers_) {
    observer.OnNotifyLocaleDidChange(locale);
  }
}

void AdsClientNotifier::NotifyPrefDidChange(const std::string& path) const {
  for (auto& observer : observers_) {
    observer.OnNotifyPrefDidChange(path);
  }
}

void AdsClientNotifier::NotifyDidUpdateResourceComponent(
    const std::string& id) const {
  for (auto& observer : observers_) {
    observer.OnNotifyDidUpdateResourceComponent(id);
  }
}

void AdsClientNotifier::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) const {
  for (auto& observer : observers_) {
    observer.OnNotifyTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientNotifier::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) const {
  for (auto& observer : observers_) {
    observer.OnNotifyTabHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientNotifier::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) const {
  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientNotifier::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) const {
  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStopPlayingMedia(tab_id);
  }
}

void AdsClientNotifier::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_visible,
    const bool is_incognito) const {
  for (auto& observer : observers_) {
    observer.OnNotifyTabDidChange(tab_id, redirect_chain, is_visible,
                                  is_incognito);
  }
}

void AdsClientNotifier::NotifyDidCloseTab(const int32_t tab_id) const {
  for (auto& observer : observers_) {
    observer.OnNotifyDidCloseTab(tab_id);
  }
}

void AdsClientNotifier::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) const {
  for (auto& observer : observers_) {
    observer.OnNotifyUserGestureEventTriggered(page_transition_type);
  }
}

void AdsClientNotifier::NotifyUserDidBecomeIdle() const {
  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeIdle();
  }
}

void AdsClientNotifier::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) const {
  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterForeground() const {
  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterForeground();
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterBackground() const {
  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterBackground();
  }
}

void AdsClientNotifier::NotifyBrowserDidBecomeActive() const {
  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidBecomeActive();
  }
}

void AdsClientNotifier::NotifyBrowserDidResignActive() const {
  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidResignActive();
  }
}

}  // namespace brave_ads
