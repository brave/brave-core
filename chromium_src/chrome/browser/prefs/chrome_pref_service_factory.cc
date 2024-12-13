/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/prefs/chrome_pref_service_factory.h"

#include "brave/components/brave_shields/core/common/pref_names.h"

#define BRAVE_TRACKED_PREFS_EXTEND                                  \
  {1000, brave_shields::prefs::kAdBlockDeveloperMode,               \
   EnforcementLevel::ENFORCE_ON_LOAD, PrefTrackingStrategy::ATOMIC, \
   ValueType::IMPERSONAL},

#include "src/chrome/browser/prefs/chrome_pref_service_factory.cc"

#undef BRAVE_TRACKED_PREFS_EXTEND
