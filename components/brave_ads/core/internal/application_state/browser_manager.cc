/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

BrowserManager::BrowserManager() {
  AddAdsClientNotifierObserver(this);
}

BrowserManager::~BrowserManager() {
  RemoveAdsClientNotifierObserver(this);
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

void BrowserManager::NotifyBrowserDidBecomeActive() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidBecomeActive();
  }
}

void BrowserManager::NotifyBrowserDidResignActive() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidResignActive();
  }
}

void BrowserManager::LogBrowserActiveState() const {
  BLOG(1, "Browser did " << (IsActive() ? "become" : "resign") << " active");
}

void BrowserManager::NotifyBrowserDidEnterForeground() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidEnterForeground();
  }
}

void BrowserManager::NotifyBrowserDidEnterBackground() const {
  for (BrowserManagerObserver& observer : observers_) {
    observer.OnBrowserDidEnterBackground();
  }
}

void BrowserManager::InitializeBrowserBackgroundState() {
  is_in_foreground_ = IsBrowserActive();

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
  if (is_active_ && *is_active_) {
    // Already active.
    return;
  }

  is_active_ = true;

  LogBrowserActiveState();

  NotifyBrowserDidBecomeActive();
}

void BrowserManager::OnNotifyBrowserDidResignActive() {
  if (is_active_ && !*is_active_) {
    // Already inactive.
    return;
  }

  is_active_ = false;

  LogBrowserActiveState();

  NotifyBrowserDidResignActive();
}

void BrowserManager::OnNotifyBrowserDidEnterForeground() {
  if (is_in_foreground_ && *is_in_foreground_) {
    // Already in foreground.
    return;
  }

  is_in_foreground_ = true;

  LogBrowserBackgroundState();

  NotifyBrowserDidEnterForeground();
}

void BrowserManager::OnNotifyBrowserDidEnterBackground() {
  if (is_in_foreground_ && !*is_in_foreground_) {
    // Already in background.
    return;
  }

  is_in_foreground_ = false;

  LogBrowserBackgroundState();

  NotifyBrowserDidEnterBackground();
}

}  // namespace brave_ads
