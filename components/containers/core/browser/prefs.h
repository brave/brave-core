// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_

#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace containers {

namespace prefs {
inline constexpr char kContainersList[] = "containers.list";
}  // namespace prefs

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

std::vector<mojom::ContainerPtr> GetContainersList(const PrefService& prefs);

void SetContainersList(PrefService& prefs,
                       const std::vector<mojom::ContainerPtr>& containers);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_PREFS_H_
