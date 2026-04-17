// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_DEFAULT_CONTAINERS_LIST_H_
#define BRAVE_BROWSER_CONTAINERS_DEFAULT_CONTAINERS_LIST_H_

#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"

namespace containers {

// Creates a list of default containers with predefined names other metadata.
std::vector<mojom::ContainerPtr> CreateDefaultContainersList();

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_DEFAULT_CONTAINERS_LIST_H_
