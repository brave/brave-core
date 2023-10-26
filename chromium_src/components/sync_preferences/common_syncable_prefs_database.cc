/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync_preferences/common_syncable_prefs_database.h"

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "components/search_engines/search_engines_pref_names.h"
// "//components/sync_preferences:common_syncable_prefs_database" already
// depends on "//components/search_engines"

namespace sync_preferences {
namespace {
namespace brave_syncable_prefs_ids {
enum {
  kSyncedDefaultPrivateSearchProviderGUID = 1000,
  kSyncedDefaultPrivateSearchProviderData = 1001
};
}  // namespace brave_syncable_prefs_ids

const auto& BraveSyncablePreferences() {
  static const auto kBraveCommonSyncablePrefsAllowlist =
      base::MakeFixedFlatMap<std::string_view, SyncablePrefMetadata>(
          {{prefs::kSyncedDefaultPrivateSearchProviderGUID,
            {brave_syncable_prefs_ids::kSyncedDefaultPrivateSearchProviderGUID,
             syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
             MergeBehavior::kNone}},
           {prefs::kSyncedDefaultPrivateSearchProviderData,
            {brave_syncable_prefs_ids::kSyncedDefaultPrivateSearchProviderData,
             syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
             MergeBehavior::kNone}}});
  return kBraveCommonSyncablePrefsAllowlist;
}
}  // namespace
}  // namespace sync_preferences

#define GetSyncablePrefMetadata GetSyncablePrefMetadata_ChromiumOriginalImpl
#include "src/components/sync_preferences/common_syncable_prefs_database.cc"
#undef GetSyncablePrefMetadata

namespace sync_preferences {

absl::optional<SyncablePrefMetadata>
CommonSyncablePrefsDatabase::GetSyncablePrefMetadata(
    const std::string& pref_name) const {
  const auto* it = BraveSyncablePreferences().find(pref_name);
  if (it != BraveSyncablePreferences().end()) {
    return it->second;
  }
  return GetSyncablePrefMetadata_ChromiumOriginalImpl(pref_name);
}

absl::optional<SyncablePrefMetadata>
CommonSyncablePrefsDatabase::GetSyncablePrefMetadata_ChromiumImpl(
    const std::string& pref_name) const {
  return GetSyncablePrefMetadata(pref_name);
}

}  // namespace sync_preferences
