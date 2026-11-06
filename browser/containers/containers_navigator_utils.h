// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_CONTAINERS_NAVIGATOR_UTILS_H_
#define BRAVE_BROWSER_CONTAINERS_CONTAINERS_NAVIGATOR_UTILS_H_

#include <optional>

#include "content/public/browser/storage_partition_config.h"

struct NavigateParams;

namespace containers {

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToInherit(const NavigateParams& params);

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_CONTAINERS_NAVIGATOR_UTILS_H_
