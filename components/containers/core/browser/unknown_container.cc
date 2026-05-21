// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/unknown_container.h"

#include <string>

#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

mojom::ContainerPtr CreateUnknownContainer(std::string_view container_id) {
  // Use first 8 characters of the container ID as the name.
  std::string_view name = container_id.substr(0, 8);
  return mojom::Container::New(std::string(container_id), std::string(name),
                               mojom::Icon::kDefault,
                               kUnknownContainerBackgroundColor);
}

}  // namespace containers
