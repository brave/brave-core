/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/partitioned_tabs/browser/storage_partition_session_info_handler.h"

#include <memory>

#include "base/containers/contains.h"
#include "base/containers/map_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/partitioned_tabs/browser/partitioned_tabs_handler.h"
#include "brave/components/partitioned_tabs/browser/session_info_utils.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"

namespace partitioned_tabs {

namespace {

// The separator between the partition domain and the partition name.
constexpr char kStoragePartitionItemSeparator[] = ":";

}  // namespace

// static
void StoragePartitionSessionInfoHandler::Register() {
  sessions::ContentSerializedNavigationDriver::GetInstance()
      ->RegisterExtendedInfoHandler(
          kStoragePartitionSessionInfoKey,
          std::make_unique<StoragePartitionSessionInfoHandler>());
}

StoragePartitionSessionInfoHandler::StoragePartitionSessionInfoHandler() =
    default;

StoragePartitionSessionInfoHandler::~StoragePartitionSessionInfoHandler() =
    default;

std::string StoragePartitionSessionInfoHandler::GetExtendedInfo(
    content::NavigationEntry* entry) const {
  if (auto config = entry->GetStoragePartitionConfig();
      config && config->partition_domain().starts_with(
                    PartitionedTabsHandler::kIdPrefix)) {
    CHECK(!base::Contains(config->partition_domain(),
                          kStoragePartitionItemSeparator));
    CHECK(!base::Contains(config->partition_name(),
                          kStoragePartitionItemSeparator));
    return base::JoinString(
        {config->partition_domain(), config->partition_name()},
        kStoragePartitionItemSeparator);
  }

  return std::string();
}

void StoragePartitionSessionInfoHandler::RestoreExtendedInfo(
    const std::string& info_string,
    content::NavigationEntry* entry) {
  // Do nothing. The storage partition config requires BrowserContext to be
  // restored. It is restored in GetStoragePartitionConfigToRestore.
}

// static
std::optional<content::StoragePartitionConfig>
StoragePartitionSessionInfoHandler::GetStoragePartitionConfigToRestore(
    content::BrowserContext* browser_context,
    const std::vector<sessions::SerializedNavigationEntry>& navigations) {
  if (navigations.empty()) {
    return std::nullopt;
  }

  const auto* extended_info = base::FindOrNull(
      navigations.front().extended_info_map(), kStoragePartitionSessionInfoKey);
  if (!extended_info ||
      !extended_info->starts_with(PartitionedTabsHandler::kIdPrefix)) {
    return std::nullopt;
  }

  const auto parts =
      base::SplitStringPiece(*extended_info, kStoragePartitionItemSeparator,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  CHECK(!parts.empty());
  const std::string_view partition_domain = parts[0];
  const std::string_view partition_name =
      parts.size() > 1 ? parts[1] : std::string_view();

  return content::StoragePartitionConfig::Create(
      browser_context, std::string(partition_domain),
      std::string(partition_name), browser_context->IsOffTheRecord());
}

}  // namespace partitioned_tabs
