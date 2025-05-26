// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_

#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace containers {

// Registers container-related preferences with the profile's preference system.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

// Returns the list of containers stored in preferences.
std::vector<mojom::ContainerPtr> GetContainerList(const PrefService& prefs);

// Stores the provided list of containers in preferences.
void SetContainerList(const std::vector<mojom::ContainerPtr>& containers,
                      PrefService& prefs);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_
