/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/search_engines_pref_names.h"
// "//components/sync_preferences:common_syncable_prefs_database" already
// depends on "//components/search_engines"

#define BRAVE_SYNCABLE_PREFS_IDS                    \
  , kSyncedDefaultPrivateSearchProviderGUID = 1000, \
    kSyncedDefaultPrivateSearchProviderData = 1001

#define BRAVE_COMMON_SYNCABLE_PREFS_ALLOW_LIST                     \
  {prefs::kSyncedDefaultPrivateSearchProviderGUID,                 \
   {syncable_prefs_ids::kSyncedDefaultPrivateSearchProviderGUID,   \
    syncer::PREFERENCES}},                                         \
  {                                                                \
    prefs::kSyncedDefaultPrivateSearchProviderData, {              \
      syncable_prefs_ids::kSyncedDefaultPrivateSearchProviderData, \
          syncer::PREFERENCES                                      \
    }                                                              \
  }

#include "src/components/sync_preferences/common_syncable_prefs_database.cc"

#undef BRAVE_COMMON_SYNCABLE_PREFS_ALLOW_LIST
#undef BRAVE_SYNCABLE_PREFS_IDS
