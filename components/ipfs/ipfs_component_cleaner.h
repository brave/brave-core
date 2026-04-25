// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_H_

#include "base/files/file_path.h"

namespace ipfs {
void CleanupIpfsComponent(const base::FilePath& user_data_dir);
}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_H_
