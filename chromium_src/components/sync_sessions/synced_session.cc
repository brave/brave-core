/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync_sessions/synced_session.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

namespace sync_sessions {
namespace {

constexpr char kStoragePartitionSessionInfoKey[] = "brave_sp";

void BraveSessionNavigationFromSyncData(
    const sync_pb::TabNavigation& sync_data,
    sessions::SerializedNavigationEntry& navigation) {
  if (!sync_data.has_brave_fields()) {
    return;
  }

  if (sync_data.brave_fields().has_storage_partition_domain()) {
    base::Value::Dict info;
    info.Set("d", sync_data.brave_fields().storage_partition_domain());
    if (sync_data.brave_fields().has_storage_partition_name()) {
      info.Set("n", sync_data.brave_fields().storage_partition_name());
    }
    if (auto info_string = base::WriteJson(info)) {
      // See storage_partition_session_info_handler.cc.
      navigation.mutable_extended_info_map()[kStoragePartitionSessionInfoKey] =
          std::move(*info_string);
    }
  }
}

void BraveSessionNavigationToSyncData(
    const sessions::SerializedNavigationEntry& navigation,
    sync_pb::TabNavigation& sync_data) {
  const auto& extended_info_map = navigation.extended_info_map();
  auto brave_sp_it = extended_info_map.find(kStoragePartitionSessionInfoKey);
  if (brave_sp_it != extended_info_map.end()) {
    if (auto brave_sp = base::JSONReader::ReadDict(brave_sp_it->second)) {
      if (auto* domain = brave_sp->FindString("d")) {
        sync_data.mutable_brave_fields()->set_storage_partition_domain(
            std::move(*domain));
      }
      if (auto* name = brave_sp->FindString("n")) {
        sync_data.mutable_brave_fields()->set_storage_partition_name(
            std::move(*name));
      }
    }
  }
}

void BraveHandleSyncDataNavigation(
    const sync_pb::TabNavigation& sync_data,
    sessions::SerializedNavigationEntry& navigation) {
  BraveSessionNavigationFromSyncData(sync_data, navigation);
}

void BraveHandleSyncDataNavigation(
    sync_pb::TabNavigation& sync_data,
    const sessions::SerializedNavigationEntry& navigation) {
  BraveSessionNavigationToSyncData(navigation, sync_data);
}

}  // namespace
}  // namespace sync_sessions

#define set_unique_id(...)    \
  set_unique_id(__VA_ARGS__); \
  BraveHandleSyncDataNavigation(sync_data, navigation)

#include "src/components/sync_sessions/synced_session.cc"

#undef set_unique_id
