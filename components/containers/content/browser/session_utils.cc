// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/session_utils.h"

#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"

namespace containers {

namespace {

// The separator between the container-encoded scheme and a URL.
constexpr std::string_view kUrlSchemeSeparator = ":";

// The separator between the partition domain and the partition name in the
// container-encoded scheme of a URL.
constexpr std::string_view kStoragePartitionKeySeparator = "+";

}  // namespace

std::optional<std::string> GetUrlPrefixForSessionPersistence(
    const std::pair<std::string, std::string>& storage_partition_key) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));

  // Only generate the prefix for container storage partitions.
  if (!IsContainersStoragePartitionKey(storage_partition_key.first,
                                       storage_partition_key.second)) {
    return std::nullopt;
  }

  return base::StrCat({
      storage_partition_key.first,
      kStoragePartitionKeySeparator,
      storage_partition_key.second,
      kUrlSchemeSeparator,
  });
}

std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
    const GURL& url,
    std::pair<std::string, std::string>& storage_partition_key,
    size_t& url_prefix_length) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));

  // Parse the URL scheme to check if it's an encoded container URL. We split on
  // '+' to separate the partition domain and partition name (UUID).
  const auto parts =
      base::SplitStringPiece(url.scheme(), kStoragePartitionKeySeparator,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  // The container scheme must have exactly 2 parts: "containers" + UUID.
  if (parts.size() != 2) {
    return std::nullopt;
  }

  // Check that this is a container storage partition key.
  if (!IsContainersStoragePartitionKey(parts[0], parts[1])) {
    return std::nullopt;
  }

  // Extract the storage partition key for restoration.
  storage_partition_key = {std::string(parts[0]), std::string(parts[1])};

  // Calculate the exact length of the prefix.
  url_prefix_length = parts[0].length() +
                      kStoragePartitionKeySeparator.length() +
                      parts[1].length() + kUrlSchemeSeparator.length();

  // Remove the prefix to get the original URL.
  return GURL(url.spec().substr(url_prefix_length));
}

}  // namespace containers
