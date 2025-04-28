// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_

#include <optional>
#include <string_view>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#include "content/public/browser/storage_partition_config.h"

namespace content {
class BrowserContext;
class SiteInstance;
class WebContents;
}  // namespace content

namespace containers {

// Returns true if the partition name is a container storage partition name.
bool IsContainerStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config);

// Returns the container id from the storage partition config if it is a
// container storage partition.
std::optional<std::string_view> GetContainerIdFromStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config);

// Returns the container storage partition config for the given browser context
// and container.
content::StoragePartitionConfig CreateContainerStoragePartition(
    content::BrowserContext* browser_context,
    const mojom::ContainerPtr& container);

// Returns the storage partition config if the given WebContents is in a
// container storage partition.
std::optional<content::StoragePartitionConfig> InheritContainerStoragePartition(
    content::WebContents* web_contents);

// Returns the storage partition config if the given storage partition config or
// site instance is in a container storage partition.
std::optional<content::StoragePartitionConfig> InheritContainerStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* site_instance);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
