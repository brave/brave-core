// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_ui_prefs.h"

#define RegisterBrowserUserPrefs(...) \
  RegisterBrowserUserPrefs_ChromiumImpl(__VA_ARGS__)

#include <chrome/browser/ui/browser_ui_prefs.cc>

#undef RegisterBrowserUserPrefs

void RegisterBrowserUserPrefs(user_prefs::PrefRegistrySyncable* registry) {
  RegisterBrowserUserPrefs_ChromiumImpl(registry);

#if defined(TOOLKIT_VIEWS)
  registry->RegisterBooleanPref(prefs::kPinShareMenuButton, true);
#endif  // defined(TOOLKIT_VIEWS)
}
