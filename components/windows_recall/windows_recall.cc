/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/windows_recall/windows_recall.h"

#include <memory>

#include "base/supports_user_data.h"
#include "base/win/windows_version.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace {

struct WindowsRecallData : public base::SupportsUserData::Data {
  static const int kKey = 0;  // Only address is used.

  explicit WindowsRecallData(const bool disabled)
      : is_windows_recall_disabled(disabled) {}

  const bool is_windows_recall_disabled = true;
};

}  // namespace

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

bool IsWindowsRecallDisabled(content::BrowserContext* browser_context) {
  if (!IsWindowsRecallAvailable()) {
    return false;
  }
  if (browser_context->IsOffTheRecord()) {
    return true;
  }

  bool disabled = true;
  if (auto* windows_recall_data = static_cast<WindowsRecallData*>(
          browser_context->GetUserData(&WindowsRecallData::kKey))) {
    disabled = windows_recall_data->is_windows_recall_disabled;
  } else {
    disabled = user_prefs::UserPrefs::Get(browser_context)
                   ->GetBoolean(prefs::kBlockWindowsRecall);
    browser_context->SetUserData(&WindowsRecallData::kKey,
                                 std::make_unique<WindowsRecallData>(disabled));
  }
  return disabled;
}

}  // namespace windows_recall
