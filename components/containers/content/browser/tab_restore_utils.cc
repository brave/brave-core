// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/tab_restore_utils.h"

#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition_config.h"

namespace containers {

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToRestore(
    content::BrowserContext* browser_context,
    base::span<const sessions::SerializedNavigationEntry> navigations,
    int selected_navigation) {
  if (!base::FeatureList::IsEnabled(features::kContainers)) {
    return std::nullopt;
  }

  if (navigations.empty()) {
    return std::nullopt;
  }

  if (selected_navigation < 0 ||
      static_cast<size_t>(selected_navigation) >= navigations.size()) {
    return std::nullopt;
  }

  if (const auto& storage_partition_key =
          navigations[selected_navigation].storage_partition_key();
      storage_partition_key &&
      IsContainersStoragePartitionKey(storage_partition_key->first,
                                      storage_partition_key->second)) {
    return content::StoragePartitionConfig::Create(
        browser_context, storage_partition_key->first,
        storage_partition_key->second, browser_context->IsOffTheRecord());
  }

  return std::nullopt;
}

}  // namespace containers
