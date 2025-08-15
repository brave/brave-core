// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/dcheck_is_on.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"

namespace containers {

ContainedTabHandlerRegistry::ContainedTabHandlerRegistry() = default;

ContainedTabHandlerRegistry::~ContainedTabHandlerRegistry() = default;

ContainedTabHandlerRegistry& ContainedTabHandlerRegistry::GetInstance() {
  static base::NoDestructor<ContainedTabHandlerRegistry> instance;
  return *instance;
}

void ContainedTabHandlerRegistry::RegisterContainedTabHandler(
    std::unique_ptr<ContainedTabHandler> handler) {
  CHECK(handler);
  CHECK(handler->GetId().starts_with(ContainedTabHandler::kIdPrefix) &&
        handler->GetId() != ContainedTabHandler::kIdPrefix)
      << handler->GetId();
#if DCHECK_IS_ON()
  for (const auto& other_handler : handlers_) {
    DCHECK_NE(handler->GetId(), other_handler->GetId())
        << "Handler " << handler->GetId() << " is already registered";
  }
#endif  // DCHECK_IS_ON()
  handlers_.push_back(std::move(handler));
}

std::optional<content::StoragePartitionConfig>
ContainedTabHandlerRegistry::MaybeInheritStoragePartition(
    content::WebContents* web_contents) {
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

  for (const auto& handler : handlers_) {
    if (handler->GetId() == storage_partition_config_ptr->partition_domain()) {
      return *storage_partition_config_ptr;
    }
  }

  return std::nullopt;
}

std::optional<GURL> ContainedTabHandlerRegistry::RestoreNavigationVirtualUrl(
    const GURL& virtual_url,
    std::string& scheme_prefix) {
  for (const auto& handler : handlers_) {
    if (virtual_url.scheme_piece() == handler->GetId()) {
      scheme_prefix = base::StrCat({handler->GetId(), ":"});
      return GURL(virtual_url.spec().substr(scheme_prefix.length()));
    }
  }
  return std::nullopt;
}

std::optional<std::string>
ContainedTabHandlerRegistry::AlterNavigationVirtualUrl(
    const GURL& virtual_url,
    const std::string& storage_partition_config) {
  if (!virtual_url.is_valid()) {
    return std::nullopt;
  }

  const auto parts =
      base::SplitStringPiece(storage_partition_config, ":",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts[0].starts_with(ContainedTabHandler::kIdPrefix) &&
      !virtual_url.spec().starts_with(parts[0])) {
    return base::StrCat({parts[0], ":", virtual_url.spec()});
  }

  return std::nullopt;
}

}  // namespace containers
