// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/partitioned_tabs/browser/partitioned_tabs_handler_registry.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "brave/components/partitioned_tabs/browser/session_info_utils.h"

#if !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#endif  // !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)

namespace partitioned_tabs {

PartitionedTabsHandlerRegistry::PartitionedTabsHandlerRegistry() = default;

PartitionedTabsHandlerRegistry::~PartitionedTabsHandlerRegistry() = default;

PartitionedTabsHandlerRegistry& PartitionedTabsHandlerRegistry::GetInstance() {
  static base::NoDestructor<PartitionedTabsHandlerRegistry> instance;
  return *instance;
}

void PartitionedTabsHandlerRegistry::RegisterPartitionedTabsHandler(
    std::unique_ptr<PartitionedTabsHandler> handler) {
  CHECK(handler);
  CHECK(handler->GetId().starts_with(PartitionedTabsHandler::kIdPrefix) &&
        handler->GetId() != PartitionedTabsHandler::kIdPrefix)
      << handler->GetId();
#if DCHECK_IS_ON()
  for (const auto& other_handler : handlers_) {
    DCHECK_NE(handler->GetId(), other_handler->GetId())
        << "Handler " << handler->GetId() << " is already registered";
  }
#endif  // DCHECK_IS_ON()
  handlers_.push_back(std::move(handler));
}

#if !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)
std::optional<content::StoragePartitionConfig>
PartitionedTabsHandlerRegistry::MaybeInheritStoragePartition(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return std::nullopt;
  }

  for (const auto& handler : handlers_) {
    if (auto config = handler->MaybeInheritStoragePartition(web_contents)) {
      return config;
    }
  }

  return std::nullopt;
}

std::optional<content::StoragePartitionConfig>
PartitionedTabsHandlerRegistry::MaybeInheritStoragePartition(
    const std::optional<content::StoragePartitionConfig>&
        storage_partition_config,
    content::SiteInstance* site_instance) {
  for (const auto& handler : handlers_) {
    if (auto config = handler->MaybeInheritStoragePartition(
            storage_partition_config, site_instance)) {
      return config;
    }
  }

  return std::nullopt;
}

bool PartitionedTabsHandlerRegistry::KeepStoragePartitionInSync(
    content::WebContents* web_contents) {
  for (const auto& handler : handlers_) {
    if (handler->KeepStoragePartitionInSync(web_contents)) {
      return true;
    }
  }

  return false;
}
#endif  // !BUILDFLAG(PARTITIONED_TABS_READ_ONLY)

bool PartitionedTabsHandlerRegistry::ShouldRestoreForeignTabFromSync(
    const std::map<std::string, std::string>& extended_info_map) {
  auto* info =
      base::FindOrNull(extended_info_map, kStoragePartitionSessionInfoKey);
  if (!info) {
    return true;
  }

  if (!info->starts_with(PartitionedTabsHandler::kIdPrefix)) {
    LOG(WARNING) << "Unsupported partitioned tabs navigation: " << *info;
    return false;
  }

  for (const auto& handler : handlers_) {
    if (info->starts_with(handler->GetId())) {
      return handler->ShouldRestoreForeignTabFromSync();
    }
  }

  // If the navigation is partitioned, but there's no handler for it, we should
  // ignore it.
  return false;
}

}  // namespace partitioned_tabs
