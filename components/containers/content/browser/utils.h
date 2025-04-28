// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_UTILS_H_

#include <optional>
#include <string>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#include "content/public/browser/storage_partition_config.h"

namespace content {
class BrowserContext;
class SiteInstance;
class WebContents;
}  // namespace content

namespace containers {

// Returns true if the partition name is a containers storage partition name.
bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config);

// Returns the container id from the containers storage partition config.
// Storage partition config must be a containers storage partition config.
std::string GetContainerIdFromStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config);

// Returns the containers storage partition config for the given browser context
// and container.
content::StoragePartitionConfig CreateContainersStoragePartition(
    content::BrowserContext* browser_context,
    const mojom::ContainerPtr& container);

// Returns the containers storage partition config if the given web contents is
// in a containers storage partition. If the web contents is not in a containers
// storage partition, returns std::nullopt.
std::optional<content::StoragePartitionConfig>
InheritContainersStoragePartition(content::WebContents* web_contents);

// Returns the containers storage partition config if the given storage
// partition config is in a containers storage partition. If the storage
// partition config is not in a containers storage partition, returns
// std::nullopt.
std::optional<content::StoragePartitionConfig>
InheritContainersStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* source_site_instance);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_UTILS_H_
