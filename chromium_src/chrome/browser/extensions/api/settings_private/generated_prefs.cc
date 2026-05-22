/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/safe_browsing/brave_generated_safe_browsing_pref.h"

// Construct Brave's subclass instead of the upstream pref so "Limited
// protection" is handled. It lives in //brave/browser to keep this chromium_src
// file free of //brave/components deps.
#define GeneratedSafeBrowsingPref BraveGeneratedSafeBrowsingPref

#include <chrome/browser/extensions/api/settings_private/generated_prefs.cc>

#undef GeneratedSafeBrowsingPref
