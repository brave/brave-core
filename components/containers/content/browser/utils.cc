// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/utils.h"

#include "brave/components/containers/core/browser/utils.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"

namespace containers {

bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config) {
  return IsContainersStoragePartitionDomain(
      storage_partition_config.partition_domain());
}

std::string GetContainerIdFromStoragePartitionConfig(
    const content::StoragePartitionConfig& storage_partition_config) {
  CHECK(IsContainersStoragePartition(storage_partition_config));
  return GetContainerIdFromContainersStoragePartitionDomain(
      storage_partition_config.partition_domain());
}

std::optional<content::StoragePartitionConfig> GetContainersStoragePartition(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return std::nullopt;
  }

  if (auto* site_instance = web_contents->GetSiteInstance()) {
    const auto& storage_partition_config =
        site_instance->GetStoragePartitionConfig();
    if (IsContainersStoragePartition(storage_partition_config)) {
      return storage_partition_config;
    }
  }

  return std::nullopt;
}

std::optional<content::StoragePartitionConfig> GetContainersStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* source_site_instance) {
  if (storage_partition_config &&
      IsContainersStoragePartition(*storage_partition_config)) {
    return storage_partition_config;
  }

  if (source_site_instance &&
      IsContainersStoragePartition(
          source_site_instance->GetStoragePartitionConfig())) {
    return source_site_instance->GetStoragePartitionConfig();
  }

  return std::nullopt;
}

content::StoragePartitionConfig GetContainersStoragePartition(
    content::BrowserContext* browser_context,
    const mojom::ContainerPtr& container) {
  return content::StoragePartitionConfig::Create(
      browser_context, GetContainersStoragePartitionDomain(container), "",
      browser_context->IsOffTheRecord());
}

}  // namespace containers
