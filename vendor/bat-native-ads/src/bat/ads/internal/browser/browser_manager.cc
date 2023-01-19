/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser/browser_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"

namespace ads {

namespace {
BrowserManager* g_browser_manager_instance = nullptr;
}  // namespace

BrowserManager::BrowserManager() {
  DCHECK(!g_browser_manager_instance);
  g_browser_manager_instance = this;

  const bool is_browser_active =
      AdsClientHelper::GetInstance()->IsBrowserActive();

  is_active_ = is_browser_active;
  LogBrowserActiveState();

  is_in_foreground_ = is_browser_active;
  LogBrowserForegroundState();
}

BrowserManager::~BrowserManager() {
  DCHECK_EQ(this, g_browser_manager_instance);
  g_browser_manager_instance = nullptr;
}

// static
BrowserManager* BrowserManager::GetInstance() {
  DCHECK(g_browser_manager_instance);
  return g_browser_manager_instance;
}

// static
bool BrowserManager::HasInstance() {
  return !!g_browser_manager_instance;
}

void BrowserManager::AddObserver(BrowserManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void BrowserManager::RemoveObserver(BrowserManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void BrowserManager::OnBrowserDidBecomeActive() {
  if (is_active_) {
    return;
  }

  is_active_ = true;
  LogBrowserActiveState();

  NotifyBrowserDidBecomeActive();
}

void BrowserManager::OnBrowserDidResignActive() {
  if (!is_active_) {
    return;
  }

  is_active_ = false;
  LogBrowserActiveState();

  NotifyBrowserDidResignActive();
}

void BrowserManager::OnBrowserDidEnterForeground() {
  if (is_in_foreground_) {
    return;
  }

  is_in_foreground_ = true;
  LogBrowserForegroundState();

  NotifyBrowserDidEnterForeground();
}

void BrowserManager::OnBrowserDidEnterBackground() {
  if (!is_in_foreground_) {
    return;
  }

  is_in_foreground_ = false;
  LogBrowserForegroundState();

  NotifyBrowserDidEnterBackground();
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
  if (is_active_) {
    BLOG(1, "Browser did become active");
  } else {
    BLOG(1, "Browser did resign active");
  }
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

void BrowserManager::LogBrowserForegroundState() const {
  if (is_in_foreground_) {
    BLOG(1, "Browser did enter foreground");
  } else {
    BLOG(1, "Browser did enter background");
  }
}

}  // namespace ads
