// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_DEFAULT_CONTAINERS_LIST_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_DEFAULT_CONTAINERS_LIST_H_

#include <array>
#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

// The default containers have fixed ids to work correctly within the Sync
// chain. If a user renames/changes icon/color, the underlying id will remain
// the same on all devices even if the device is not Synced yet.
//
// DO NOT CHANGE THESE IDs, only deprecate and add new ones if necessary.
inline constexpr auto kDefaultContainerIds = std::to_array({
    "0053d88e-7b26-4bfa-9175-783e1bfcba97",
    "2c2777a1-80b3-4b26-b629-ae1aefd9f272",
    "cfc3ee90-a163-4bd0-99ff-f0a472dda804",
    "f5d49086-1a5f-42b0-83fa-58682e5b8ba5",
});

// Creates a list of default containers with localized names and Nala-driven
// icon background colors.
std::vector<mojom::ContainerPtr> CreateDefaultContainersList();

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_DEFAULT_CONTAINERS_LIST_H_
