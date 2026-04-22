// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_TEMPORARY_CONTAINER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_TEMPORARY_CONTAINER_H_

#include <string_view>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"

namespace containers {

// Prefix for temporary container ids.
inline constexpr std::string_view kTemporaryContainerIdPrefix = "t-";

// Checks if a given container id is a temporary container id.
bool IsTemporaryContainerId(std::string_view container_id);

// Builds a temporary container (`t-` prefixed container id, BIP-39 two-word
// name, random icon, random background color).
mojom::ContainerPtr CreateTemporaryContainer();

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_TEMPORARY_CONTAINER_H_
