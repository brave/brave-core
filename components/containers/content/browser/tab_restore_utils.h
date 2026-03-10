// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_TAB_RESTORE_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_TAB_RESTORE_UTILS_H_

#include <optional>

#include "base/component_export.h"
#include "base/containers/span.h"

namespace content {
class BrowserContext;
class StoragePartitionConfig;
}  // namespace content

namespace sessions {
class SerializedNavigationEntry;
}  // namespace sessions

namespace containers {

// Returns the StoragePartitionConfig to use when restoring a tab from session
// data. Examines the selected navigation entry for container storage partition
// info and, if found, creates the corresponding StoragePartitionConfig.
// Returns std::nullopt if the Containers feature is disabled, navigations are
// empty, the selected index is out of range, or the navigation doesn't belong
// to a container.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER_TAB_RESTORE)
std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToRestore(
    content::BrowserContext* browser_context,
    base::span<const sessions::SerializedNavigationEntry> navigations,
    int selected_navigation);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_TAB_RESTORE_UTILS_H_
