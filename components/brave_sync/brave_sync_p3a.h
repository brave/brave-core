/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_P3A_H_

#include "components/sync/base/user_selectable_type.h"

namespace brave_sync {
namespace p3a {

// TODO(alexeybarabash): move here also "Brave.Sync.Status.2" and
// "Brave.Sync.ProgressTokenEverReset"
inline constexpr char kEnabledTypesHistogramName[] = "Brave.Sync.EnabledTypes";
// Improved version of metric which includes count of synced History objects
inline constexpr char kSyncedObjectsCountHistogramNameV2[] =
    "Brave.Sync.SyncedObjectsCount.2";
inline constexpr char kSyncJoinTypeHistogramName[] = "Brave.Sync.JoinType";

enum class EnabledTypesAnswer {
  kEmptyOrBookmarksOnly = 0,
  kBookmarksAndHistory = 1,
  kMoreThanBookmarksAndHistory = 2,
  kAllTypes = 3,
  kMaxValue = kAllTypes
};

enum class SyncJoinType {
  kChainCreated = 1,
  kChainJoined = 2,
  kMaxValue = kChainJoined
};

void RecordEnabledTypes(bool sync_everything_enabled,
                        const syncer::UserSelectableTypeSet& selected_types);
void RecordSyncedObjectsCount(int total_entities);

// Monitors sync code generation and setting events in order
// to report the `Brave.Sync.JoinType` metric.
class SyncCodeMonitor {
 public:
  SyncCodeMonitor() = default;
  ~SyncCodeMonitor() = default;

  SyncCodeMonitor(const SyncCodeMonitor&) = delete;
  SyncCodeMonitor& operator=(const SyncCodeMonitor&) = delete;

  void RecordCodeGenerated();
  void RecordCodeSet();

 private:
  bool code_generated_ = false;
};

}  // namespace p3a
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_P3A_H_
