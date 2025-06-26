/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_H_
#define BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_H_

#include "base/memory/raw_ptr.h"

class PrefRegistrySimple;
class PrefService;

namespace windows_recall {

namespace prefs {
inline constexpr char kWindowsRecallDisabled[] =
    "brave.windows_recall_disabled";
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

// Checks whether the Window Recall service functionality is potentially
// available; currently, this verifies if the version is Windows 11 or newer.
bool IsWindowsRecallAvailable();

// Gets the current state of the setting in the browser.
// NOTE: This is not related to system settings and does not check the status of
// the system service.
bool IsWindowsRecallDisabled(PrefService* local_state);

namespace test {

class ScopedWindowsRecallDisabledOverride {
 public:
  explicit ScopedWindowsRecallDisabledOverride(bool disabled);
  ~ScopedWindowsRecallDisabledOverride();

 private:
  bool disabled_ = false;
  raw_ptr<ScopedWindowsRecallDisabledOverride> original_instance_ = nullptr;
  static ScopedWindowsRecallDisabledOverride* instance_;
};

}  // namespace test

}  // namespace windows_recall

#endif  // BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_H_
