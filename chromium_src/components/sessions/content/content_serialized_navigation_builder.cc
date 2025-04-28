/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sessions/content/content_serialized_navigation_builder.h"

#include <string>

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "base/containers/map_util.h"
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#define FromNavigationEntry FromNavigationEntry_ChromiumImpl

#if BUILDFLAG(ENABLE_CONTAINERS)
// If the SerializedNavigationEntry contains storage partition key information ,
// we copy it into the NavigationEntry so it can be used later to create the
// correct SiteInstance with the right StoragePartitionConfig.
//
// This happens AFTER ContentSerializedNavigationDriver::Sanitize() has already
// parsed the virtual URL prefix and populated the storage_partition_key field.
#define BRAVE_CONTENT_SERIALIZED_NAVIGATION_BUILDER_TO_NAVIGATION_ENTRY  \
  if (base::FeatureList::IsEnabled(containers::features::kContainers) && \
      navigation->storage_partition_key().has_value()) {                 \
    entry->SetStoragePartitionKeyToRestore(                              \
        navigation->storage_partition_key().value());                    \
  }
#else
#define BRAVE_CONTENT_SERIALIZED_NAVIGATION_BUILDER_TO_NAVIGATION_ENTRY
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <components/sessions/content/content_serialized_navigation_builder.cc>

#undef FromNavigationEntry
#undef BRAVE_CONTENT_SERIALIZED_NAVIGATION_BUILDER_TO_NAVIGATION_ENTRY

namespace sessions {

// This method is called when saving a session (browser close, tab close, manual
// session save, or sync). We override it to add storage partition key
// information to the serialized data.
//
// The SerializedNavigationEntry will then be processed by
// ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle() which
// adds the prefix to the PageState, and finally written to disk/sync with the
// virtual_url_prefix embedded in the virtual_url field.
SerializedNavigationEntry
ContentSerializedNavigationBuilder::FromNavigationEntry(
    int index,
    content::NavigationEntry* entry,
    SerializationOptions serialization_options) {
  SerializedNavigationEntry navigation =
      FromNavigationEntry_ChromiumImpl(index, entry, serialization_options);

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    if (auto storage_partition_key_to_restore =
            entry->GetStoragePartitionKeyToRestore()) {
      // Generate the URL prefix (e.g., "containers+<uuid>:") if it's a valid
      // container storage partition key.
      if (auto url_prefix = containers::GetUrlPrefixForSessionPersistence(
              *storage_partition_key_to_restore)) {
        // Store both the prefix and the raw partition key in the
        // SerializedNavigationEntry. The prefix will be used by
        // GetSanitizedPageStateForPickle() to modify the PageState,
        // and both will be embedded in the virtual_url during
        // final serialization.
        navigation.set_virtual_url_prefix(*url_prefix);
        navigation.set_storage_partition_key(*storage_partition_key_to_restore);
      }
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return navigation;
}

}  // namespace sessions
