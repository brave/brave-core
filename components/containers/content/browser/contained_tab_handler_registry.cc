// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/dcheck_is_on.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"

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

ContainedTabHandlerRegistry::ContainedTabHandlerRegistry() = default;

ContainedTabHandlerRegistry::~ContainedTabHandlerRegistry() = default;

ContainedTabHandlerRegistry& ContainedTabHandlerRegistry::GetInstance() {
  static base::NoDestructor<ContainedTabHandlerRegistry> instance;
  return *instance;
}

void ContainedTabHandlerRegistry::RegisterContainedTabHandler(
    std::unique_ptr<ContainedTabHandler> handler) {
  CHECK(handler);
#if DCHECK_IS_ON()
  DCHECK(handler->GetId().starts_with(ContainedTabHandler::kIdPrefix) &&
         handler->GetId() != ContainedTabHandler::kIdPrefix)
      << handler->GetId();
  for (const auto& other_handler : handlers_) {
    DCHECK_NE(handler->GetId(), other_handler->GetId())
        << "Handler " << handler->GetId() << " is already registered";
  }
#endif  // DCHECK_IS_ON()
  handlers_.push_back(std::move(handler));
}

bool ContainedTabHandlerRegistry::ShouldInheritStoragePartition(
    const content::StoragePartitionConfig& partition_config) const {
  return std::ranges::any_of(
      handlers_, [&partition_config](const auto& handler) {
        return handler->GetId() == partition_config.partition_domain();
      });
}

std::optional<content::StoragePartitionConfig>
ContainedTabHandlerRegistry::MaybeInheritStoragePartition(
    content::WebContents* web_contents) const {
  if (!web_contents) {
    return std::nullopt;
  }

  return MaybeInheritStoragePartition(std::nullopt,
                                      web_contents->GetSiteInstance());
}

std::optional<content::StoragePartitionConfig>
ContainedTabHandlerRegistry::MaybeInheritStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* site_instance) const {
  const content::StoragePartitionConfig* storage_partition_config_ptr = nullptr;
  if (storage_partition_config) {
    storage_partition_config_ptr = &storage_partition_config.value();
  } else if (site_instance) {
    storage_partition_config_ptr = &site_instance->GetStoragePartitionConfig();
  }

  if (!storage_partition_config_ptr) {
    return std::nullopt;
  }

  if (ShouldInheritStoragePartition(*storage_partition_config_ptr)) {
    return *storage_partition_config_ptr;
  }

  return std::nullopt;
}

std::optional<std::string> ContainedTabHandlerRegistry::GetVirtualUrlPrefix(
    const std::pair<std::string, std::string>& storage_partition_key) {
  for (const auto& handler : handlers_) {
    if (handler->GetId() == storage_partition_key.first) {
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
  }

  return std::nullopt;
}

std::optional<GURL>
ContainedTabHandlerRegistry::RestoreStoragePartitionKeyFromUrl(
    const GURL& url,
    std::pair<std::string, std::string>& storage_partition_key,
    size_t& url_prefix_length) {
  const auto parts =
      base::SplitStringPiece(url.scheme(), kStoragePartitionKeySeparator,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (parts.size() != 2) {
    return std::nullopt;
  }

  for (const auto& handler : handlers_) {
    if (parts[0] == handler->GetId()) {
      storage_partition_key = {std::string(parts[0]), std::string(parts[1])};
      url_prefix_length =
          parts[0].length() + kStoragePartitionKeySeparator.length() +
          parts[1].length() + kVirtualUrlSchemeSeparator.length();
      return GURL(url.spec().substr(url_prefix_length));
    }
  }

  return std::nullopt;
}

}  // namespace containers
