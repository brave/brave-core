/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/content/browser/storage_partition_session_info_handler.h"

#include <memory>

#include "base/containers/contains.h"
#include "base/containers/map_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/containers/content/browser/contained_tab_handler.h"
#include "brave/components/containers/content/browser/session_info_key.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"

namespace containers {

namespace {

// The separator between the partition domain and the partition name.
constexpr char kStoragePartitionItemSeparator[] = ":";

// Serializes the storage partition config to the extended info string.
std::string SerializeStoragePartitionConfig(
    const std::pair<std::string, std::string>& config) {
  if (!config.first.starts_with(ContainedTabHandler::kIdPrefix)) {
    // Only serialize the config if it is a partitioned tab.
    return std::string();
  }

  CHECK(!base::Contains(config.first, kStoragePartitionItemSeparator));
  CHECK(!config.second.empty());
  CHECK(!base::Contains(config.second, kStoragePartitionItemSeparator));
  return base::JoinString({config.first, config.second},
                          kStoragePartitionItemSeparator);
}

// Parses the storage partition config from the extended info string.
// Returns std::nullopt if the config is invalid.
std::optional<std::pair<std::string, std::string>> ParseStoragePartitionConfig(
    const std::string& info_string) {
  auto parts =
      base::SplitStringPiece(info_string, kStoragePartitionItemSeparator,
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() != 2 ||
      !parts[0].starts_with(ContainedTabHandler::kIdPrefix) ||
      parts[1].empty()) {
    LOG(WARNING) << "Invalid storage partition config to restore: "
                 << info_string;
    return std::nullopt;
  }

  return std::make_pair(std::string(parts[0]), std::string(parts[1]));
}

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
  if (auto config = entry->GetStoragePartitionConfigToRestore(); config) {
    return SerializeStoragePartitionConfig(*config);
  }

  return std::string();
}

void StoragePartitionSessionInfoHandler::RestoreExtendedInfo(
    const std::string& info_string,
    content::NavigationEntry* entry) {
  if (auto config = ParseStoragePartitionConfig(info_string)) {
    entry->SetStoragePartitionToRestore(config->first, config->second);
  }
}

// static
std::optional<content::StoragePartitionConfig>
StoragePartitionSessionInfoHandler::GetStoragePartitionConfigToRestore(
    content::BrowserContext* browser_context,
    const std::vector<sessions::SerializedNavigationEntry>& navigations,
    int selected_navigation) {
  if (navigations.empty()) {
    return std::nullopt;
  }

  if (selected_navigation < 0 ||
      static_cast<size_t>(selected_navigation) >= navigations.size()) {
    return std::nullopt;
  }

  const auto* extended_info =
      base::FindOrNull(navigations.at(selected_navigation).extended_info_map(),
                       kStoragePartitionSessionInfoKey);
  if (!extended_info) {
    return std::nullopt;
  }

  if (auto config = ParseStoragePartitionConfig(*extended_info)) {
    return content::StoragePartitionConfig::Create(
        browser_context, config->first, config->second,
        browser_context->IsOffTheRecord());
  }

  return std::nullopt;
}

}  // namespace containers
