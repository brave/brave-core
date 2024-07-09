// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_H_

#include "base/files/file_path.h"

namespace ipfs {

base::FilePath::StringPieceType GetIpfsClientComponentId();

base::FilePath GetIpfsClientComponentPath();

void DeleteIpfsComponent(const base::FilePath& component_path);
}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_H_
