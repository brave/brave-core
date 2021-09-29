/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser_manager/browser_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/user_activity/user_activity.h"

namespace ads {

namespace {
BrowserManager* g_browser_manager = nullptr;
}  // namespace

BrowserManager::BrowserManager() {
  DCHECK_EQ(g_browser_manager, nullptr);
  g_browser_manager = this;
}

BrowserManager::~BrowserManager() {
  DCHECK(g_browser_manager);
  g_browser_manager = nullptr;
}

// static
BrowserManager* BrowserManager::Get() {
  DCHECK(g_browser_manager);
  return g_browser_manager;
}

// static
bool BrowserManager::HasInstance() {
  return g_browser_manager;
}

void BrowserManager::SetActive(const bool is_active) {
  is_active_ = is_active;
}

bool BrowserManager::IsActive() const {
  return is_active_ && IsForegrounded();
}

void BrowserManager::OnActive() {
  if (is_active_) {
    return;
  }

  BLOG(1, "Browser window is active");

  is_active_ = true;

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserWindowIsActive);
}

void BrowserManager::OnInactive() {
  if (!is_active_) {
    return;
  }

  BLOG(1, "Browser window is inactive");

  is_active_ = false;

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserWindowIsInactive);
}

void BrowserManager::SetForegrounded(const bool is_foregrounded) {
  is_foregrounded_ = is_foregrounded;
}

bool BrowserManager::IsForegrounded() const {
  return is_foregrounded_;
}

void BrowserManager::OnForegrounded() {
  if (is_foregrounded_) {
    return;
  }

  BLOG(1, "Browser did become active");

  is_foregrounded_ = true;

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserDidBecomeActive);
}

void BrowserManager::OnBackgrounded() {
  if (!is_foregrounded_) {
    return;
  }

  BLOG(1, "Browser did enter background");

  is_foregrounded_ = false;

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserDidEnterBackground);
}

}  // namespace ads
