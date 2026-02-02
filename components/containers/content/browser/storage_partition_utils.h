// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_

#include <optional>

#include "base/component_export.h"
#include "base/types/optional_ref.h"

namespace content {
class StoragePartitionConfig;
}  // namespace content

namespace containers {

// The partition domain identifier used for all containers storage partitions.
inline constexpr char kContainersStoragePartitionDomain[] =
    "containers-default";

// Checks whether a given StoragePartitionConfig belongs to Containers.
// Partition domain should match kContainersStoragePartitionDomain and partition
// name should be non-empty.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config);

// Returns the StoragePartitionConfig if it is a Containers storage partition,
// otherwise returns std::nullopt. Used to conditionally inherit
// StoragePartitionConfig when creating a new SiteInstance.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    base::optional_ref<const content::StoragePartitionConfig>
        storage_partition_config);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
