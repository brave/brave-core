/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/browser/renderer_host/navigation_entry_impl.cc>

namespace content {

void NavigationEntryImpl::SetStoragePartitionKeyToRestore(
    std::pair<std::string, std::string> storage_partition_key) {
  storage_partition_key_to_restore_ = std::move(storage_partition_key);
}

base::optional_ref<const std::pair<std::string, std::string>>
NavigationEntryImpl::GetStoragePartitionKeyToRestore() {
  if (frame_tree_ && frame_tree_->frame_entry) {
    if (auto* site_instance = frame_tree_->frame_entry->site_instance()) {
      const auto& storage_partition_config =
          site_instance->GetStoragePartitionConfig();
      if (!storage_partition_key_to_restore_ ||
          storage_partition_key_to_restore_->first !=
              storage_partition_config.partition_domain() ||
          storage_partition_key_to_restore_->second !=
              storage_partition_config.partition_name()) {
        storage_partition_key_to_restore_ = {
            storage_partition_config.partition_domain(),
            storage_partition_config.partition_name(),
        };
      }
    }
  }

  return storage_partition_key_to_restore_;
}

}  // namespace content
