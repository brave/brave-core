// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/types/optional_ref.h"

namespace content {
class StoragePartitionConfig;
class WebContents;
}  // namespace content

namespace containers {

// The partition domain identifier used for all containers storage partitions.
inline constexpr char kContainersStoragePartitionDomain[] = "containers";

// Checks whether a given StoragePartitionConfig belongs to Containers.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config);

// Checks whether a given StoragePartitionConfig partition domain and name
// belongs to Containers.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsContainersStoragePartitionKey(std::string_view partition_domain,
                                     std::string_view partition_name);

// Checks whether a storage partition key component is not empty and contains
// only valid characters for Containers storage partitions. Valid characters are
// ASCII alphanumeric characters and '-'.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsValidStoragePartitionKeyComponent(std::string_view component);

// Returns the StoragePartitionConfig if it is a Containers storage partition,
// otherwise returns std::nullopt. Used to conditionally inherit
// StoragePartitionConfig when creating a new SiteInstance.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    base::optional_ref<const content::StoragePartitionConfig>
        storage_partition_config);

// Returns partition_name() when |partition_config| is a Containers storage
// partition; otherwise an empty view. The view is only valid for the lifetime
// of data owned by |partition_config|.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::string_view GetContainerIdFromStoragePartitionConfig(
    const content::StoragePartitionConfig& partition_config LIFETIME_BOUND);

// Returns the container ID for |web_contents|: first from SiteInstance when it
// identifies a Containers storage partition; otherwise from WebContentsUserData
// attached after tab discard when the stub WebContents does not yet have the
// correct SiteInstance. Returns an empty string if |web_contents| is null, or
// if neither source yields a valid Containers partition.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::string GetContainerIdForWebContents(content::WebContents* web_contents);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
