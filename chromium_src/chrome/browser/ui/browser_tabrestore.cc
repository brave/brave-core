// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_tabrestore.h"

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/tab_contents/tab_util.h"

#define GetSiteInstanceForNewTab(...)                  \
  GetSiteInstanceForNewTab(                            \
      __VA_ARGS__, GetStoragePartitionConfigToRestore( \
                       browser->profile(), navigations, selected_navigation))

namespace {

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToRestore(
    content::BrowserContext* browser_context,
    base::span<const sessions::SerializedNavigationEntry> navigations,
    int selected_navigation) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
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
      containers::IsContainersStoragePartitionKey(
          storage_partition_key->first, storage_partition_key->second)) {
    return content::StoragePartitionConfig::Create(
        browser_context, storage_partition_key->first,
        storage_partition_key->second, browser_context->IsOffTheRecord());
  }

  return std::nullopt;
}

}  // namespace
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/browser_tabrestore.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
