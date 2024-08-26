// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_IPFS_IPFS_COMPONENT_CLEANER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_IPFS_IPFS_COMPONENT_CLEANER_DELEGATE_IMPL_H_

#include "base/files/file_path.h"
#include "brave/components/ipfs/ipfs_component_cleaner_delegate.h"

namespace ipfs {

class IpfsComponentCleanerDelegateImpl : public IpfsComponentCleanerDelegate {
 public:
  ~IpfsComponentCleanerDelegateImpl() override;

  base::FilePath::StringPieceType GetIpfsClientComponentId() override;
  base::FilePath GetIpfsClientComponentPath() override;
  void DeleteIpfsComponent(const base::FilePath& component_path) override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_COMPONENT_CLEANER_DELEGATE_IMPL_H_
