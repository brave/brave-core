/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
// To avoid conflicts with the macro from the Windows SDK...
#undef GetUserName
#endif

#define MigrateObsoleteProfileAttributes   \
  BraveMigrateObsoleteProfileAttributes(); \
  void MigrateObsoleteProfileAttributes

#define RecordAccountNamesMetric              \
  RecordAccountNamesMetric_UnUsed() {}        \
  friend class ProfileAttributeMigrationTest; \
  void RecordAccountNamesMetric

#include "src/chrome/browser/profiles/profile_attributes_entry.h"  // IWYU pragma: export
#undef MigrateObsoleteProfileAttributes
#undef RecordAccountNamesMetric

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
