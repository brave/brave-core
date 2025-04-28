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
#include "brave/components/containers/content/browser/session_info_key.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#define FromNavigationEntry FromNavigationEntry_ChromiumImpl

#include <components/sessions/content/content_serialized_navigation_builder.cc>

#undef FromNavigationEntry

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
  const auto* storage_partition_info =
      base::FindOrNull(navigation.extended_info_map_,
                       containers::kStoragePartitionSessionInfoKey);
  if (storage_partition_info) {
    if (auto storage_partition_config =
            containers::ContainedTabHandlerRegistry::GetInstance()
                .AlterNavigationVirtualUrl(navigation.virtual_url_,
                                           *storage_partition_info)) {
      navigation.virtual_url_ = GURL(*storage_partition_config);
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return navigation;
}

}  // namespace sessions
