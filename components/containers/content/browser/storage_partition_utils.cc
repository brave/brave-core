// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "content/public/browser/storage_partition_config.h"

namespace containers {

bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config) {
  return partition_config.partition_domain() ==
             kContainersStoragePartitionDomain &&
         !partition_config.partition_name().empty();
}

std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    base::optional_ref<const content::StoragePartitionConfig>
        storage_partition_config) {
  if (storage_partition_config &&
      IsContainersStoragePartition(*storage_partition_config)) {
    return *storage_partition_config;
  }

  return std::nullopt;
}

}  // namespace containers
