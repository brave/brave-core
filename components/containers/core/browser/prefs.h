// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_

#include <string_view>
#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"

class PrefService;

namespace containers {

// Returns the list of containers stored in preferences.
std::vector<mojom::ContainerPtr> GetContainersFromPrefs(
    const PrefService& prefs);

// Returns the container with `id` from the synced containers preferences, or a
// null mojom::ContainerPtr if it is not present.
mojom::ContainerPtr GetContainerFromPrefs(const PrefService& prefs,
                                          std::string_view id);

// Stores the provided list of containers in preferences.
void SetContainersToPrefs(const std::vector<mojom::ContainerPtr>& containers,
                          PrefService& prefs);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_
