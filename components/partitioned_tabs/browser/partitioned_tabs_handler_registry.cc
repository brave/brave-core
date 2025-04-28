// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/partitioned_tabs/browser/partitioned_tabs_handler_registry.h"

#include <memory>
#include <vector>

#include "base/containers/map_util.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "brave/components/partitioned_tabs/browser/session_info_handler.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"

namespace partitioned_tabs {

PartitionedTabsHandlerRegistry::PartitionedTabsHandlerRegistry() = default;

PartitionedTabsHandlerRegistry::~PartitionedTabsHandlerRegistry() = default;

PartitionedTabsHandlerRegistry& PartitionedTabsHandlerRegistry::GetInstance() {
  static base::NoDestructor<PartitionedTabsHandlerRegistry> instance;
  return *instance;
}

void PartitionedTabsHandlerRegistry::RegisterPartitionedTabsHandler(
    std::unique_ptr<PartitionedTabsHandler> handler) {
  handlers_.push_back(std::move(handler));
}

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

bool PartitionedTabsHandlerRegistry::ShouldRestoreForeignTabFromSync(
    const std::map<std::string, std::string>& extended_info_map) {
  for (const auto& handler : handlers_) {
    if (handler->ShouldRestoreForeignTabFromSync(extended_info_map)) {
      return true;
    }
  }

  // If the tab is partitioned, we should keep it from sync.
  if (auto* info = base::FindOrNull(
          extended_info_map,
          SessionInfoHandler::kStoragePartitionSessionInfoKey)) {
    if (auto json = base::JSONReader::ReadDict(*info)) {
      const auto* domain = json->FindString("d");
      if (domain && domain->starts_with("partitioned-")) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace partitioned_tabs
