// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/ipfs_component_cleaner.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "brave/components/ipfs/ipfs_common.h"
#include "build/build_config.h"

namespace ipfs {
void CleanupIpfsComponent(const base::FilePath& user_data_dir) {
  // Remove IPFS component
  base::ThreadPool::PostTask(
      FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
      base::GetDeletePathRecursivelyCallback(
          user_data_dir.Append(kIpfsClientComponentId)));
}
}  // namespace ipfs
