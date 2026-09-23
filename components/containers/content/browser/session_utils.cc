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

RestoreStoragePartitionKeyResult::RestoreStoragePartitionKeyResult(
    const GURL& url,
    const std::pair<std::string, std::string>& storage_partition_key,
    size_t url_prefix_length)
    : url(url),
      storage_partition_key(storage_partition_key),
      url_prefix_length(url_prefix_length) {}
RestoreStoragePartitionKeyResult::~RestoreStoragePartitionKeyResult() = default;
RestoreStoragePartitionKeyResult::RestoreStoragePartitionKeyResult(
    const RestoreStoragePartitionKeyResult&) = default;
RestoreStoragePartitionKeyResult::RestoreStoragePartitionKeyResult(
    RestoreStoragePartitionKeyResult&&) noexcept = default;
RestoreStoragePartitionKeyResult& RestoreStoragePartitionKeyResult::operator=(
    const RestoreStoragePartitionKeyResult&) = default;
RestoreStoragePartitionKeyResult& RestoreStoragePartitionKeyResult::operator=(
    RestoreStoragePartitionKeyResult&&) noexcept = default;

std::optional<std::string> StoragePartitionKeyToUrlPrefix(
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

std::optional<RestoreStoragePartitionKeyResult>
RestoreStoragePartitionKeyFromUrl(const GURL& url) {
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

  // Calculate the exact length of the prefix.
  const size_t url_prefix_length =
      parts[0].length() + kStoragePartitionKeySeparator.length() +
      parts[1].length() + kUrlSchemeSeparator.length();

  return RestoreStoragePartitionKeyResult(
      GURL(url.spec().substr(url_prefix_length)),
      {std::string(parts[0]), std::string(parts[1])}, url_prefix_length);
}

}  // namespace containers
