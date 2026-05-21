// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_USED_CONTAINER_STORAGE_PARTITIONS_H_
#define BRAVE_BROWSER_CONTAINERS_USED_CONTAINER_STORAGE_PARTITIONS_H_

#include <vector>

#include "content/public/browser/storage_partition_config.h"

class Profile;

namespace containers {

// Returns StoragePartitionConfig for each locally used container.
std::vector<content::StoragePartitionConfig>
GetUsedContainerStoragePartitionConfigs(Profile* profile);

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_USED_CONTAINER_STORAGE_PARTITIONS_H_
