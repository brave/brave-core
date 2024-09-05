// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_COMMON_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_COMMON_H_

#include "base/files/file_path.h"
#include "build/build_config.h"

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

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_COMMON_H_
