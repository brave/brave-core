/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

BrowserManager::BrowserManager() {
  GetAdsClient().AddObserver(this);
}

BrowserManager::~BrowserManager() {
  GetAdsClient().RemoveObserver(this);
}

// static
BrowserManager& BrowserManager::GetInstance() {
  return GlobalState::GetInstance()->GetBrowserManager();
}

void BrowserManager::AddObserver(BrowserManagerObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void BrowserManager::RemoveObserver(BrowserManagerObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

bool BrowserManager::IsCurrentlyActive() const {
  return is_active_.has_value() && *is_active_;
}

void BrowserManager::NotifyBrowserDidBecomeActive() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidBecomeActive();
  }
}

bool BrowserManager::IsCurrentlyInactive() const {
  return is_active_.has_value() && !*is_active_;
}

void BrowserManager::NotifyBrowserDidResignActive() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidResignActive();
  }
}

void BrowserManager::LogBrowserActiveState() const {
  BLOG(1, "Browser did " << (IsActive() ? "become" : "resign") << " active");
}

bool BrowserManager::IsCurrentlyInForeground() const {
  return is_in_foreground_.has_value() && *is_in_foreground_;
}

void BrowserManager::NotifyBrowserDidEnterForeground() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidEnterForeground();
  }
}

bool BrowserManager::IsCurrentlyInBackground() const {
  return is_in_foreground_.has_value() && !*is_in_foreground_;
}

void BrowserManager::NotifyBrowserDidEnterBackground() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidEnterBackground();
  }
}

void BrowserManager::InitializeBrowserBackgroundState() {
  is_in_foreground_ = GetAdsClient().IsBrowserActive();

  LogBrowserBackgroundState();
}

void BrowserManager::LogBrowserBackgroundState() const {
  BLOG(1, "Browser did enter "
              << (IsInForeground() ? "foreground" : "background"));
}

void BrowserManager::OnNotifyDidInitializeAds() {
  InitializeBrowserBackgroundState();
}

void BrowserManager::OnNotifyBrowserDidBecomeActive() {
  if (IsCurrentlyActive()) {
    return;
  }

  is_active_ = true;
  LogBrowserActiveState();
  NotifyBrowserDidBecomeActive();
}

void BrowserManager::OnNotifyBrowserDidResignActive() {
  if (IsCurrentlyInactive()) {
    return;
  }

  is_active_ = false;
  LogBrowserActiveState();
  NotifyBrowserDidResignActive();
}

void BrowserManager::OnNotifyBrowserDidEnterForeground() {
  if (IsCurrentlyInForeground()) {
    return;
  }

  is_in_foreground_ = true;
  LogBrowserBackgroundState();
  NotifyBrowserDidEnterForeground();
}

void BrowserManager::OnNotifyBrowserDidEnterBackground() {
  if (IsCurrentlyInBackground()) {
    return;
  }

  is_in_foreground_ = false;
  LogBrowserBackgroundState();
  NotifyBrowserDidEnterBackground();
}

}  // namespace brave_ads
