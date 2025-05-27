/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/windows_recall/windows_recall_service.h"

#include "brave/components/windows_recall/windows_recall.h"
#include "brave/components/windows_recall/windows_recall_service_factory.h"
#include "components/prefs/pref_service.h"

namespace windows_recall {

WindowsRecallService::WindowsRecallService(PrefService* pref_service)
    : is_windows_recall_enabled_(
          !pref_service->GetBoolean(prefs::kBlockWindowsRecall)),
      pref_service_(pref_service) {}

WindowsRecallService::~WindowsRecallService() = default;

// static
WindowsRecallService* WindowsRecallService::Get(
    content::BrowserContext* browser_context) {
  return WindowsRecallServiceFactory::GetForBrowserContext(browser_context);
}

bool WindowsRecallService::IsWindowsRecallEnabled() const {
  return is_windows_recall_enabled_;
}

void WindowsRecallService::EnableWindowsRecall(bool enable) {
  pref_service_->SetBoolean(prefs::kBlockWindowsRecall, !enable);
}

}  // namespace windows_recall
