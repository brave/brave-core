/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TYPES_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TYPES_H_

#include <string>
#include <utility>

#include "content/public/browser/storage_partition_config.h"

namespace ephemeral_storage {

// TLD ephemeral area is keyed by the TLD-specific security domain and
// StoragePartitionConfig.
using TLDEphemeralAreaKey =
    std::pair<std::string, content::StoragePartitionConfig>;

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TYPES_H_
