/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/windows_recall/windows_recall.h"

#include "base/win/windows_version.h"
#include "brave/components/windows_recall/windows_recall_service.h"
#include "components/prefs/pref_registry_simple.h"

namespace windows_recall {

bool IsWindowsRecallAvailable() {
  return base::win::GetVersion() >= base::win::Version::WIN11;
}

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  if (!IsWindowsRecallAvailable()) {
    return;
  }

  registry->RegisterBooleanPref(prefs::kBlockWindowsRecall, true);
}

WindowsRecallState GetWindowsRecallState(
    content::BrowserContext* browser_context) {
  if (!IsWindowsRecallAvailable()) {
    return WindowsRecallState::kUnavailable;
  }

  auto* service = WindowsRecallService::Get(browser_context);
  if (!service) {
    return WindowsRecallState::kUnavailable;
  }
  return service->IsWindowsRecallEnabled() ? WindowsRecallState::kEnabled
                                           : WindowsRecallState::kDisabled;
}

}  // namespace windows_recall
