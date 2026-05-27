// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINER_SPECIFIER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINER_SPECIFIER_H_

#include <string>
#include <variant>

#include "base/types/strong_alias.h"

namespace containers {

// Strong alias for container id.
using ContainerId = base::StrongAlias<class ContainerIdTag, std::string>;
// Strong alias for container name.
using ContainerName = base::StrongAlias<class ContainerNameTag, std::string>;

// A variant of container id or name used to represent a container.
using ContainerSpecifier =
    std::variant<std::monostate, ContainerId, ContainerName>;

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINER_SPECIFIER_H_
