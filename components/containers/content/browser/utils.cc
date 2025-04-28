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

std::string GetContainerIdFromStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config) {
  CHECK(IsContainersStoragePartition(storage_partition_config));
  return GetContainerIdFromContainersStoragePartitionDomain(
      storage_partition_config.partition_domain());
}

content::StoragePartitionConfig CreateContainersStoragePartition(
    content::BrowserContext* browser_context,
    const mojom::ContainerPtr& container) {
  return content::StoragePartitionConfig::Create(
      browser_context, GetContainersStoragePartitionDomain(container), "",
      browser_context->IsOffTheRecord());
}

std::optional<content::StoragePartitionConfig>
InheritContainersStoragePartition(content::WebContents* web_contents) {
  if (!web_contents) {
    return std::nullopt;
  }

  return InheritContainersStoragePartition(std::nullopt,
                                           web_contents->GetSiteInstance());
}

std::optional<content::StoragePartitionConfig>
InheritContainersStoragePartition(
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

}  // namespace containers
