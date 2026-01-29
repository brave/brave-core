// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"

namespace containers {

bool IsContainersStoragePartition(
    const content::StoragePartitionConfig& partition_config) {
  return IsContainersStoragePartitionKey(partition_config.partition_domain(),
                                         partition_config.partition_name());
}

bool IsContainersStoragePartitionKey(std::string_view partition_domain,
                                     std::string_view partition_name) {
  return partition_domain == kContainersStoragePartitionDomain &&
         !partition_name.empty();
}

std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return std::nullopt;
  }

  return MaybeInheritStoragePartition(std::nullopt,
                                      web_contents->GetSiteInstance());
}

std::optional<content::StoragePartitionConfig> MaybeInheritStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* site_instance) {
  const content::StoragePartitionConfig* storage_partition_config_ptr = nullptr;
  if (storage_partition_config) {
    storage_partition_config_ptr = &storage_partition_config.value();
  } else if (site_instance) {
    storage_partition_config_ptr = &site_instance->GetStoragePartitionConfig();
  }

  if (!storage_partition_config_ptr) {
    return std::nullopt;
  }

  if (IsContainersStoragePartition(*storage_partition_config_ptr)) {
    return *storage_partition_config_ptr;
  }

  return std::nullopt;
}

// std::optional<std::string> GetVirtualUrlPrefix(
//     const std::pair<std::string, std::string>& storage_partition_key) {
//   for (const auto& handler : handlers_) {
//     if (handler->GetId() == storage_partition_key.first) {
//       CHECK(IsValidStoragePartitionKeyItem(storage_partition_key.first))
//           << storage_partition_key.first;
//       CHECK(IsValidStoragePartitionKeyItem(storage_partition_key.second))
//           << storage_partition_key.second;
//       return base::StrCat({
//           storage_partition_key.first,
//           kStoragePartitionKeySeparator,
//           storage_partition_key.second,
//           kVirtualUrlSchemeSeparator,
//       });
//     }
//   }

//   return std::nullopt;
// }

// std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
//     const GURL& url,
//     std::pair<std::string, std::string>& storage_partition_key,
//     size_t& url_prefix_length) {
//   const auto parts =
//       base::SplitStringPiece(url.scheme(), kStoragePartitionKeySeparator,
//                              base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

//   if (parts.size() != 2) {
//     return std::nullopt;
//   }

//   for (const auto& handler : handlers_) {
//     if (parts[0] == handler->GetId()) {
//       storage_partition_key = {std::string(parts[0]), std::string(parts[1])};
//       url_prefix_length =
//           parts[0].length() + kStoragePartitionKeySeparator.length() +
//           parts[1].length() + kVirtualUrlSchemeSeparator.length();
//       return GURL(url.spec().substr(url_prefix_length));
//     }
//   }

//   return std::nullopt;
// }

}  // namespace containers
