/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check_op.h"

#include <content/browser/renderer_host/navigation_entry_impl.cc>

namespace content {

// SetStoragePartitionKeyToRestore and GetStoragePartitionKeyToRestore are
// added to content::NavigationEntry to persist and restore the storage
// partition key (partition_domain, partition_name) that should be used when
// this navigation entry is restored.
//
// NavigationEntry has a reference to a SiteInstance, which has a
// StoragePartitionConfig. However, when restoring from a serialized session:
// 1. The NavigationEntry is created before the SiteInstance.
// 2. We need to pass the correct StoragePartitionConfig when creating the
//    SiteInstance.
// 3. Standard Chromium doesn't serialize this information.
//
// These methods bridge that gap by storing the "intended" storage partition
// key for serialization and restoration:
// - GetStoragePartitionKeyToRestore(): Returns the partition key for
//   serialization AND restoration.
// - SetStoragePartitionKeyToRestore(): Sets the partition key from deserialized
//   data.
//
// The storage partition key is a simplified representation of
// StoragePartitionConfig, containing just (partition_domain, partition_name).

void NavigationEntryImpl::SetStoragePartitionKeyToRestore(
    std::pair<std::string, std::string> storage_partition_key) {
  storage_partition_key_to_restore_ = std::move(storage_partition_key);
}

// Gets the storage partition key that should be used for this entry.
//
// Called during session serialization (FromNavigationEntry) when saving to disk
// and when creating a SiteInstance for navigation.
//
// If SiteInstance does not exist (entry being restored or not yet navigated),
// return the stored key from deserialization, which will be used to create a
// SiteInstance with the correct StoragePartitionConfig.
base::optional_ref<const std::pair<std::string, std::string>>
NavigationEntryImpl::GetStoragePartitionKeyToRestore() {
  // Try to sync with the current SiteInstance's partition config.
  // frame_tree_->frame_entry->site_instance() will be null during initial
  // restoration but populated after the navigation completes.
  if (auto* site_instance = (frame_tree_ && frame_tree_->frame_entry)
                                ? frame_tree_->frame_entry->site_instance()
                                : nullptr) {
    const auto& storage_partition_config =
        site_instance->GetStoragePartitionConfig();

    // Set the storage partition key to be used during session serialization.
    if (!storage_partition_key_to_restore_) {
      storage_partition_key_to_restore_ = {
          storage_partition_config.partition_domain(),
          storage_partition_config.partition_name(),
      };
    } else {
      // Assert the stored key matches the SiteInstance's partition config if we
      // get here multiple times.
      DUMP_WILL_BE_CHECK_EQ(storage_partition_key_to_restore_->first,
                            storage_partition_config.partition_domain());
      DUMP_WILL_BE_CHECK_EQ(storage_partition_key_to_restore_->second,
                            storage_partition_config.partition_name());
    }
  }

  return storage_partition_key_to_restore_;
}

}  // namespace content
