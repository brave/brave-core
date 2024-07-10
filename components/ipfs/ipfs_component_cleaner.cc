// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/ipfs_component_cleaner.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"

namespace {
#if BUILDFLAG(IS_WIN)
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("lnbclahgobmjphilkalbhebakmblnbij");
#elif BUILDFLAG(IS_MAC)
#if defined(ARCH_CPU_ARM64)
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("lejaflgbgglfaomemffoaappaihfligf");
#else
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("nljcddpbnaianmglkpkneakjaapinabi");
#endif
#elif BUILDFLAG(IS_LINUX)
#if defined(ARCH_CPU_ARM64)
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("fmmldihckdnognaabhligdpckkeancng");
#else
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("oecghfpdmkjlhnfpmmjegjacfimiafjp");
#endif
#endif
}  // namespace

namespace ipfs {

base::FilePath::StringPieceType GetIpfsClientComponentId() {
  return kIpfsClientComponentId;
}

base::FilePath GetIpfsClientComponentPath() {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return user_data_dir.Append(GetIpfsClientComponentId());
}

void DeleteIpfsComponent(const base::FilePath& component_path) {
  if (component_path.empty()) {
    return;
  }
  // Remove IPFS component
  base::ThreadPool::PostTask(
      FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
      base::BindOnce(IgnoreResult(&base::DeletePathRecursively),
                     component_path));
}

}  // namespace ipfs
