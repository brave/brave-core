// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_UNKNOWN_CONTAINER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_UNKNOWN_CONTAINER_H_

#include <string_view>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#include "third_party/skia/include/core/SkColor.h"

namespace containers {

// Background color to represent an unknown container.
inline constexpr SkColor kUnknownContainerBackgroundColor =
    SkColorSetRGB(0xb7, 0x4d, 0x49);

// Builds an "unknown" Container for the given `container_id`. The unknown
// container is a placeholder for a container that is not present in the synced
// and locally used lists. This exists to handle unknown containers gracefully
// at runtime.
mojom::ContainerPtr CreateUnknownContainer(std::string_view container_id);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_UNKNOWN_CONTAINER_H_
