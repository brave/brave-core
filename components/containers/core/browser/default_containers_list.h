// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_DEFAULT_CONTAINERS_LIST_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_DEFAULT_CONTAINERS_LIST_H_

#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

// Creates a list of default containers with localized names and Nala-driven
// icon background colors.
std::vector<mojom::ContainerPtr> CreateDefaultContainersList();

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_DEFAULT_CONTAINERS_LIST_H_
