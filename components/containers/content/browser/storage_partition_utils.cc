// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include <algorithm>

#include "base/strings/string_util.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"

namespace containers {

bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return IsContainersStoragePartitionKey(partition_config.partition_domain(),
                                         partition_config.partition_name());
}

bool IsContainersStoragePartitionKey(std::string_view partition_domain,
                                     std::string_view partition_name) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return partition_domain == kContainersStoragePartitionDomain &&
         IsValidStoragePartitionKeyComponent(partition_name);
}

bool IsValidStoragePartitionKeyComponent(std::string_view component) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  return !component.empty() &&
         std::ranges::all_of(component, [](const char& c) {
           return base::IsAsciiAlphaNumeric(c) || c == '-';
         });
}

std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    base::optional_ref<const content::StoragePartitionConfig>
        storage_partition_config) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  if (storage_partition_config &&
      IsContainersStoragePartition(*storage_partition_config)) {
    return *storage_partition_config;
  }

  return std::nullopt;
}

std::string GetContainerIdForWebContents(content::WebContents* web_contents) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  if (!web_contents) {
    return std::string();
  }

  const auto& config =
      web_contents->GetSiteInstance()->GetStoragePartitionConfig();
  if (!IsContainersStoragePartition(config)) {
    return std::string();
  }

  return config.partition_name();
}

}  // namespace containers
