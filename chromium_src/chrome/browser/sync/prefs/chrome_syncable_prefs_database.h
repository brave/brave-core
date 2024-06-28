/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_PREFS_CHROME_SYNCABLE_PREFS_DATABASE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_PREFS_CHROME_SYNCABLE_PREFS_DATABASE_H_

#include <optional>

#include "components/sync_preferences/common_syncable_prefs_database.h"
#include "components/sync_preferences/syncable_prefs_database.h"

#define GetSyncablePrefMetadata                                           \
  GetSyncablePrefMetadata_ChromiumImpl(std::string_view pref_name) const; \
  std::optional<sync_preferences::SyncablePrefMetadata> GetSyncablePrefMetadata

#include "src/chrome/browser/sync/prefs/chrome_syncable_prefs_database.h"  // IWYU pragma: export
#undef GetSyncablePrefMetadata

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_PREFS_CHROME_SYNCABLE_PREFS_DATABASE_H_
