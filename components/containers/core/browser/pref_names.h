// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREF_NAMES_H_

namespace containers::prefs {

// Controls whether Containers feature (menus, management UI) is available;
// existing container tabs are not affected when the pref is disabled.
inline constexpr char kContainersEnabled[] = "brave.containers.enabled";

// Syncable list of containers.
inline constexpr char kContainersList[] = "brave.containers.list";

// Local-only dictionary of container snapshots that are still referenced by
// this profile even if they disappear from the synced containers list.
inline constexpr char kLocallyUsedContainers[] = "brave.containers.used";
// Dict holding at most one of {container_id: string, temporary_container: true,
// ask_each_time: true}: where blank new tabs (Ctrl+T, the new-tab button)
// open. Empty means no container. Local, like kLocallyUsedContainers.
inline constexpr char kNewTabTarget[] = "brave.containers.new_tab_target";

}  // namespace containers::prefs

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREF_NAMES_H_
