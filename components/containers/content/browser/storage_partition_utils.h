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

inline constexpr char kContainersStoragePartitionDomain[] =
    "containers-default";

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config);

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    base::optional_ref<const content::StoragePartitionConfig>
        storage_partition_config);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
