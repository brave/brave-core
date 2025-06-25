/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/windows_recall/windows_recall.h"

#include "base/check_is_test.h"
#include "base/win/windows_version.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace windows_recall {

namespace {
bool* GetCurrentDisabledState() {
  static bool disabled = false;
  return &disabled;
}

}  // namespace

bool IsWindowsRecallAvailable() {
  return base::win::GetVersion() >= base::win::Version::WIN11;
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  if (!IsWindowsRecallAvailable()) {
    return;
  }

  registry->RegisterBooleanPref(prefs::kWindowsRecallDisabled, true);
}

bool IsWindowsRecallDisabled(PrefService* local_state) {
  if (!IsWindowsRecallAvailable()) {
    return false;
  }

  bool* disabled = GetCurrentDisabledState();
  *disabled = local_state->GetBoolean(prefs::kWindowsRecallDisabled);
  return *disabled;
}

void SetCurrentDisabledStateForTesting(bool disabled) {
  CHECK_IS_TEST();
  *GetCurrentDisabledState() = disabled;
}

}  // namespace windows_recall
