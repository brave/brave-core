// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "brave/components/containers/core/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"

namespace containers {

bool IsContainerStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return IsContainerStoragePartitionDomain(
      storage_partition_config.partition_domain());
}

std::optional<std::string_view> GetContainerIdFromStoragePartition(
    const content::StoragePartitionConfig& storage_partition_config) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return GetContainerIdFromStoragePartitionDomain(
      storage_partition_config.partition_domain());
}

content::StoragePartitionConfig CreateContainerStoragePartition(
    content::BrowserContext* browser_context,
    const mojom::ContainerPtr& container) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return content::StoragePartitionConfig::Create(
      browser_context, GetContainerStoragePartitionDomain(container), "",
      browser_context->IsOffTheRecord());
}

std::optional<content::StoragePartitionConfig> InheritContainerStoragePartition(
    content::WebContents* web_contents) {
  if (!base::FeatureList::IsEnabled(features::kContainers)) {
    return std::nullopt;
  }

  if (!web_contents) {
    return std::nullopt;
  }

  return InheritContainerStoragePartition(std::nullopt,
                                          web_contents->GetSiteInstance());
}

std::optional<content::StoragePartitionConfig> InheritContainerStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* site_instance) {
  if (!base::FeatureList::IsEnabled(features::kContainers)) {
    return std::nullopt;
  }

  if (storage_partition_config &&
      IsContainerStoragePartition(*storage_partition_config)) {
    return storage_partition_config;
  }

  if (site_instance &&
      IsContainerStoragePartition(site_instance->GetStoragePartitionConfig())) {
    return site_instance->GetStoragePartitionConfig();
  }

  return std::nullopt;
}

}  // namespace containers
