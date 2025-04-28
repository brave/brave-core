/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/browser/renderer_host/navigation_entry_impl.cc"

namespace content {

base::optional_ref<const StoragePartitionConfig>
NavigationEntryImpl::GetStoragePartitionConfig() {
  if (!frame_tree_ || !frame_tree_->frame_entry) {
    return std::nullopt;
  }
  if (auto* site_instance = frame_tree_->frame_entry->site_instance()) {
    return site_instance->GetStoragePartitionConfig();
  }
  return std::nullopt;
}

}  // namespace content
