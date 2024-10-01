/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_p3a.h"

#include "brave/components/p3a_utils/bucket.h"
#include "components/sync/base/user_selectable_type.h"

namespace brave_sync::p3a {

void RecordEnabledTypes(bool sync_everything_enabled,
                        const syncer::UserSelectableTypeSet& selected_types) {
  using syncer::UserSelectableType;
  using syncer::UserSelectableTypeSet;

  EnabledTypesAnswer sample;
  static const auto all_brave_supported_types = UserSelectableTypeSet(
      {UserSelectableType::kBookmarks, UserSelectableType::kHistory,
       UserSelectableType::kExtensions, UserSelectableType::kApps,
       UserSelectableType::kPasswords, UserSelectableType::kPreferences,
       UserSelectableType::kThemes, UserSelectableType::kTabs,
       UserSelectableType::kAutofill});
  if (sync_everything_enabled ||
      selected_types.HasAll(all_brave_supported_types)) {
    // Sync All
    sample = EnabledTypesAnswer::kAllTypes;
  } else if (selected_types.empty() ||
             selected_types ==
                 UserSelectableTypeSet({UserSelectableType::kBookmarks})) {
    // Bookmarks or no types
    sample = EnabledTypesAnswer::kEmptyOrBookmarksOnly;
  } else if (selected_types ==
             UserSelectableTypeSet({UserSelectableType::kBookmarks,
                                    UserSelectableType::kHistory})) {
    // Bookmarks & History
    sample = EnabledTypesAnswer::kBookmarksAndHistory;
  } else {
    // More than (Bookmarks & History) but less than (Sync All)
    sample = EnabledTypesAnswer::kMoreThanBookmarksAndHistory;
  }

  base::UmaHistogramEnumeration(kEnabledTypesHistogramName, sample);
}

void RecordSyncedObjectsCount(int total_entities) {
  // "Brave.Sync.SyncedObjectsCount.2"
  // 0 - 0..1000
  // 1 - 1001..10000
  // 2 - 10001..49000
  // 3 - >= 49001
  p3a_utils::RecordToHistogramBucket(kSyncedObjectsCountHistogramNameV2,
                                     {1000, 10000, 49000}, total_entities);
}

void SyncCodeMonitor::RecordCodeGenerated() {
  code_generated_ = true;
  base::UmaHistogramEnumeration(kSyncJoinTypeHistogramName,
                                SyncJoinType::kChainCreated);
}

void SyncCodeMonitor::RecordCodeSet() {
  if (!code_generated_) {
    base::UmaHistogramEnumeration(kSyncJoinTypeHistogramName,
                                  SyncJoinType::kChainJoined);
  }
  code_generated_ = false;
}

}  // namespace brave_sync::p3a
