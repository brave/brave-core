// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/sessions/content/content_serialized_navigation_builder.h"

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/browser/navigation_entry.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace {

// Copies the storage partition key parsed from the SerializedNavigationEntry
// into the NavigationEntry so it can be used later to create the correct
// SiteInstance with the right StoragePartitionConfig.
//
// This runs AFTER ContentSerializedNavigationDriver::Sanitize() has already
// parsed the virtual URL prefix and populated the storage_partition_key field.
void MaybeRestoreStoragePartitionKey(
    [[maybe_unused]] const sessions::SerializedNavigationEntry* navigation,
    [[maybe_unused]] content::NavigationEntry* entry) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers) &&
      navigation->storage_partition_key().has_value()) {
    entry->SetStoragePartitionKeyToRestore(
        navigation->storage_partition_key().value());
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

// Stores the NavigationEntry's storage partition key on the
// SerializedNavigationEntry when saving a session. The
// SerializedNavigationEntry is then processed by
// ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle() which
// adds the prefix to the PageState, and finally written to disk/sync with the
// virtual_url_prefix embedded in the virtual_url field.
void MaybeSaveStoragePartitionKey(
    [[maybe_unused]] content::NavigationEntry* entry,
    [[maybe_unused]] sessions::SerializedNavigationEntry* navigation) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return;
  }
  auto storage_partition_key_to_restore =
      entry->GetStoragePartitionKeyToRestore();
  if (!storage_partition_key_to_restore) {
    return;
  }
  // Generate the URL prefix (e.g., "containers+<uuid>:") if it's a valid
  // container storage partition key.
  auto url_prefix = containers::StoragePartitionKeyToUrlPrefix(
      *storage_partition_key_to_restore);
  if (!url_prefix) {
    return;
  }
  // Store both the prefix and the raw partition key in the
  // SerializedNavigationEntry. The prefix will be used by
  // GetSanitizedPageStateForPickle() to modify the PageState, and both will be
  // embedded in the virtual_url during final serialization.
  navigation->set_virtual_url_prefix(*url_prefix);
  navigation->set_storage_partition_key(*storage_partition_key_to_restore);
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

}  // namespace

#include <components/sessions/content/content_serialized_navigation_builder.cc>
