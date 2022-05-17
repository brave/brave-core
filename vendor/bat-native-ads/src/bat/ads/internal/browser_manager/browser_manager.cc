/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser_manager/browser_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/base/logging_util.h"

namespace ads {

namespace {
BrowserManager* g_browser_manager_instance = nullptr;
}  // namespace

BrowserManager::BrowserManager() {
  DCHECK(!g_browser_manager_instance);
  g_browser_manager_instance = this;
}

BrowserManager::~BrowserManager() {
  DCHECK_EQ(this, g_browser_manager_instance);
  g_browser_manager_instance = nullptr;
}

// static
BrowserManager* BrowserManager::Get() {
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

void BrowserManager::OnDidBecomeActive() {
  if (is_active_) {
    return;
  }

  BLOG(1, "Browser did become active");

  is_active_ = true;

  NotifyBrowserDidBecomeActive();
}

void BrowserManager::OnDidResignActive() {
  if (!is_active_) {
    return;
  }

  BLOG(1, "Browser did resign active");

  is_active_ = false;

  NotifyBrowserDidResignActive();
}

void BrowserManager::OnDidEnterForeground() {
  if (is_foreground_) {
    return;
  }

  BLOG(1, "Browser did enter foreground");

  is_foreground_ = true;

  NotifyBrowserDidEnterForeground();
}

void BrowserManager::OnDidEnterBackground() {
  if (!is_foreground_) {
    return;
  }

  BLOG(1, "Browser did enter background");

  is_foreground_ = false;

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

}  // namespace ads
