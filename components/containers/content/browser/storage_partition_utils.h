// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_

#include <optional>
#include <string_view>

#include "base/component_export.h"

namespace content {
class SiteInstance;
class StoragePartitionConfig;
class WebContents;
}  // namespace content

namespace containers {

inline constexpr char kContainersStoragePartitionDomain[] =
    "containers-default";

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config);

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
bool IsContainersStoragePartitionKey(std::string_view partition_domain,
                                     std::string_view partition_name);

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    content::WebContents* web_contents);

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* site_instance);

// std::optional<std::string> GetVirtualUrlPrefix(
//     const std::pair<std::string, std::string>& storage_partition_key);

// std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
//     const GURL& url,
//     std::pair<std::string, std::string>& storage_partition_key,
//     size_t& url_prefix_length);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_STORAGE_PARTITION_UTILS_H_
