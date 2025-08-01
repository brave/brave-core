/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync_sessions/synced_session.h"

#include "brave/components/partitioned_tabs/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#include "base/containers/map_util.h"
#include "brave/components/partitioned_tabs/browser/partitioned_tabs_handler_registry.h"
#include "brave/components/partitioned_tabs/browser/session_info_utils.h"

namespace sync_sessions {
namespace {

void ReadNavigationStoragePartitionFromSyncData(
    const sync_pb::TabNavigation& sync_data,
    sessions::SerializedNavigationEntry& navigation) {
  if (!sync_data.has_brave_fields() ||
      !sync_data.brave_fields().has_storage_partition_config()) {
    return;
  }

  const auto& storage_partition_config =
      sync_data.brave_fields().storage_partition_config();

  navigation.mutable_extended_info_map()
      [partitioned_tabs::kStoragePartitionSessionInfoKey] =
      storage_partition_config;

  // if (auto restored_virtual_url =
  //         partitioned_tabs::PartitionedTabsHandlerRegistry::GetInstance()
  //             .MaybeRestoreSyncNavigationVirtualUrl(sync_data.virtual_url())) {
  //   navigation.set_virtual_url(GURL(*restored_virtual_url));
  // }
}

void WriteNavigationStoragePartitionToSyncData(
    const sessions::SerializedNavigationEntry& navigation,
    sync_pb::TabNavigation& sync_data) {
  const auto& extended_info_map = navigation.extended_info_map();
  const auto* storage_partition_info = base::FindOrNull(
      extended_info_map, partitioned_tabs::kStoragePartitionSessionInfoKey);
  if (!storage_partition_info) {
    return;
  }

  sync_data.mutable_brave_fields()->set_storage_partition_config(
      *storage_partition_info);

  // if (auto altered_virtual_url =
  //         partitioned_tabs::PartitionedTabsHandlerRegistry::GetInstance()
  //             .MaybeAlterSyncNavigationVirtualUrl(navigation.virtual_url(),
  //                                                 *storage_partition_info)) {
  //   sync_data.set_virtual_url(*altered_virtual_url);
  // }
}

void BraveHandleSyncDataNavigation(
    const sync_pb::TabNavigation& sync_data,
    sessions::SerializedNavigationEntry& navigation) {
  ReadNavigationStoragePartitionFromSyncData(sync_data, navigation);
}

void BraveHandleSyncDataNavigation(
    sync_pb::TabNavigation& sync_data,
    const sessions::SerializedNavigationEntry& navigation) {
  WriteNavigationStoragePartitionToSyncData(navigation, sync_data);
}

}  // namespace
}  // namespace sync_sessions

#define set_virtual_url(...)    \
  set_virtual_url(__VA_ARGS__); \
  BraveHandleSyncDataNavigation(sync_data, navigation)

#endif

#include <components/sync_sessions/synced_session.cc>

#if BUILDFLAG(ENABLE_PARTITIONED_TABS)
#undef set_virtual_url
#endif
