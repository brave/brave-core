// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/session_utils.h"

#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"

namespace containers {

namespace {

// The separator between the partition domain and the partition name.
constexpr std::string_view kStoragePartitionKeySeparator = "+";

// The separator between the scheme and the virtual URL.
constexpr std::string_view kVirtualUrlSchemeSeparator = ":";

bool IsValidStoragePartitionKeyItem(std::string_view item) {
  return std::all_of(item.begin(), item.end(), [](const char& c) {
    return base::IsAsciiAlphaNumeric(c) || c == '-';
  });
}

}  // namespace

std::optional<std::string> GetVirtualUrlPrefix(
    const std::pair<std::string, std::string>& storage_partition_key) {
  if (!IsContainersStoragePartitionKey(storage_partition_key.first,
                                       storage_partition_key.second)) {
    return std::nullopt;
  }

  CHECK(IsValidStoragePartitionKeyItem(storage_partition_key.first))
      << storage_partition_key.first;
  CHECK(IsValidStoragePartitionKeyItem(storage_partition_key.second))
      << storage_partition_key.second;
  return base::StrCat({
      storage_partition_key.first,
      kStoragePartitionKeySeparator,
      storage_partition_key.second,
      kVirtualUrlSchemeSeparator,
  });
}

std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
    const GURL& url,
    std::pair<std::string, std::string>& storage_partition_key,
    size_t& url_prefix_length) {
  const auto parts =
      base::SplitStringPiece(url.scheme(), kStoragePartitionKeySeparator,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (parts.size() != 2) {
    return std::nullopt;
  }

  if (!IsContainersStoragePartitionKey(parts[0], parts[1])) {
    return std::nullopt;
  }

  storage_partition_key = {std::string(parts[0]), std::string(parts[1])};
  url_prefix_length = parts[0].length() +
                      kStoragePartitionKeySeparator.length() +
                      parts[1].length() + kVirtualUrlSchemeSeparator.length();
  return GURL(url.spec().substr(url_prefix_length));
}

}  // namespace containers
