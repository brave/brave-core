/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/containers/storage_partition_session_info_handler.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "content/public/browser/navigation_entry.h"

namespace containers {

namespace {
constexpr char kStoragePartitionSessionInfoKey[] = "brave_sp";
}

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
  base::Value::Dict info;

  if (auto config = entry->GetStoragePartitionConfig();
      config && !config->partition_domain().empty()) {
    info.Set("d", config->partition_domain());
    if (!config->partition_name().empty()) {
      info.Set("n", config->partition_name());
    }
  }

  if (info.empty()) {
    return std::string();
  }

  return base::WriteJson(info).value_or(std::string());
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

  const auto& extended_info_map = navigations.front().extended_info_map();
  auto extended_info_it =
      extended_info_map.find(kStoragePartitionSessionInfoKey);
  if (extended_info_it == extended_info_map.end() ||
      extended_info_it->second.empty()) {
    return std::nullopt;
  }

  std::optional<base::Value::Dict> info =
      base::JSONReader::ReadDict(extended_info_it->second);
  if (!info) {
    return std::nullopt;
  }

  auto* partition_domain = info->FindString("d");
  if (!partition_domain || partition_domain->empty()) {
    return std::nullopt;
  }

  auto* partition_name = info->FindString("n");
  return content::StoragePartitionConfig::Create(
      browser_context, *partition_domain,
      partition_name ? *partition_name : std::string(), false);
}

}  // namespace containers
