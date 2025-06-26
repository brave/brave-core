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
bool* g_windows_recall_disabled_override_for_testing = nullptr;
}

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

  static bool disabled = local_state->GetBoolean(prefs::kWindowsRecallDisabled);

  if (g_windows_recall_disabled_override_for_testing) {
    CHECK_IS_TEST();
    return *g_windows_recall_disabled_override_for_testing;
  }

  return disabled;
}

namespace test {

// static
ScopedWindowsRecallDisabledOverride*
    ScopedWindowsRecallDisabledOverride::instance_ = nullptr;

ScopedWindowsRecallDisabledOverride::ScopedWindowsRecallDisabledOverride(
    bool disabled)
    : disabled_(disabled), original_instance_(instance_) {
  CHECK_IS_TEST();
  instance_ = this;
  g_windows_recall_disabled_override_for_testing = &instance_->disabled_;
}

ScopedWindowsRecallDisabledOverride::~ScopedWindowsRecallDisabledOverride() {
  instance_ = original_instance_;
  g_windows_recall_disabled_override_for_testing =
      instance_ ? &instance_->disabled_ : nullptr;
}

}  // namespace test

}  // namespace windows_recall
