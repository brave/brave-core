/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// This is done to allow the same renaming in
// chromium_src/chrome/browser/prefs/browser_prefs.cc
#include "components/translate/core/browser/translate_prefs.h"

#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#include "src/components/translate/core/browser/translate_prefs.cc"
#undef MigrateObsoleteProfilePrefs
