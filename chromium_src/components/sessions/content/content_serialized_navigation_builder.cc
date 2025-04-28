/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sessions/content/content_serialized_navigation_builder.h"

#include <string>

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "base/containers/map_util.h"
#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#define FromNavigationEntry FromNavigationEntry_ChromiumImpl

#define BRAVE_CONTENT_SERIALIZED_NAVIGATION_BUILDER_TO_NAVIGATION_ENTRY \
  if (navigation->storage_partition_key().has_value()) {                \
    entry->SetStoragePartitionKeyToRestore(                             \
        navigation->storage_partition_key().value());                   \
  }

#include <components/sessions/content/content_serialized_navigation_builder.cc>

#undef FromNavigationEntry
#undef BRAVE_CONTENT_SERIALIZED_NAVIGATION_BUILDER_TO_NAVIGATION_ENTRY

namespace sessions {

// static
SerializedNavigationEntry
ContentSerializedNavigationBuilder::FromNavigationEntry(
    int index,
    content::NavigationEntry* entry,
    SerializationOptions serialization_options) {
  SerializedNavigationEntry navigation =
      FromNavigationEntry_ChromiumImpl(index, entry, serialization_options);

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (auto storage_partition_key_to_restore =
          entry->GetStoragePartitionKeyToRestore()) {
    if (auto virtual_url_prefix =
            containers::ContainedTabHandlerRegistry::GetInstance()
                .GetVirtualUrlPrefix(*storage_partition_key_to_restore)) {
      navigation.set_virtual_url_prefix(*virtual_url_prefix);
      navigation.set_storage_partition_key(*storage_partition_key_to_restore);
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return navigation;
}

}  // namespace sessions
