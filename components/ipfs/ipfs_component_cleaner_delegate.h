// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_DELEGATE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_DELEGATE_H_

#include "base/files/file_path.h"

namespace ipfs {

class IpfsComponentCleanerDelegate {
 public:
  virtual ~IpfsComponentCleanerDelegate() = default;

  virtual base::FilePath::StringPieceType GetIpfsClientComponentId() = 0;
  virtual base::FilePath GetIpfsClientComponentPath() = 0;
  virtual void DeleteIpfsComponent(const base::FilePath& component_path) = 0;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_COMPONENT_CLEANER_DELEGATE_H_
