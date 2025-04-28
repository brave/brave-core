// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_STORAGE_PARTITION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_STORAGE_PARTITION_UTILS_H_

#include <optional>
#include <string>
#include <string_view>

#include "brave/components/containers/core/mojom/containers.mojom-forward.h"

namespace containers {

// Returns true if the partition domain is a container storage partition domain.
bool IsContainerStoragePartitionDomain(std::string_view partition_domain);

// Returns the container id from the storage partition domain if it is a
// container storage partition domain.
std::optional<std::string_view> GetContainerIdFromStoragePartitionDomain(
    std::string_view partition_domain);

// Returns the container storage partition domain for the given container.
std::string GetContainerStoragePartitionDomain(
    const mojom::ContainerPtr& container);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_STORAGE_PARTITION_UTILS_H_
