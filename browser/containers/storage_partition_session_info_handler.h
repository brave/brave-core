/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CONTAINERS_STORAGE_PARTITION_SESSION_INFO_HANDLER_H_
#define BRAVE_BROWSER_CONTAINERS_STORAGE_PARTITION_SESSION_INFO_HANDLER_H_

#include <optional>
#include <string>
#include <vector>

#include "components/sessions/content/extended_info_handler.h"
#include "content/public/browser/storage_partition_config.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace sessions {
class SerializedNavigationEntry;
}  // namespace sessions

namespace containers {

// Handles the extra session info that defines the storage partition.
class StoragePartitionSessionInfoHandler
    : public sessions::ExtendedInfoHandler {
 public:
  // Creates and registers a single instance.
  static void Register();

  StoragePartitionSessionInfoHandler();
  ~StoragePartitionSessionInfoHandler() override;

  // ExtendedInfoHandler:
  std::string GetExtendedInfo(content::NavigationEntry* entry) const override;
  void RestoreExtendedInfo(const std::string& info,
                           content::NavigationEntry* entry) override;

  static std::optional<content::StoragePartitionConfig>
  GetStoragePartitionConfigToRestore(
      content::BrowserContext* browser_context,
      const std::vector<sessions::SerializedNavigationEntry>& navigations);
};

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_STORAGE_PARTITION_SESSION_INFO_HANDLER_H_
