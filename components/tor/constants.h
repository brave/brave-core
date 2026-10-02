/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_CONSTANTS_H_
#define BRAVE_COMPONENTS_TOR_CONSTANTS_H_

#include <cstdint>
#include <iterator>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/safe_base_name.h"
#include "build/build_config.h"
#include "crypto/sha2.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Windows)";
inline constexpr char kTorClientComponentId[] =
    "cpoalefficncklhjfpglfiplenlpccdb";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0x2f, 0xe0, 0xb4, 0x55, 0x82, 0xd2, 0xab, 0x79, 0x5f, 0x6b, 0x58,
    0xfb, 0x4d, 0xbf, 0x22, 0x31, 0xdd, 0x3b, 0xf4, 0xf6, 0x50, 0x9e,
    0xdb, 0xd4, 0xec, 0xa7, 0xef, 0x63, 0xed, 0x9d, 0x0f, 0xc7};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_MAC)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Mac)";
inline constexpr char kTorClientComponentId[] =
    "cldoidikboihgcjfkhdeidbpclkineef";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0x2b, 0x3e, 0x83, 0x8a, 0x1e, 0x87, 0x62, 0x95, 0xa7, 0x34, 0x83,
    0x1f, 0x2b, 0xa8, 0xd4, 0x45, 0x6c, 0x2b, 0xea, 0x12, 0x65, 0x2f,
    0x2e, 0xce, 0xfa, 0x83, 0x65, 0xd5, 0x33, 0x2c, 0x13, 0xb2};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_LINUX)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Linux)";
#if defined(ARCH_CPU_ARM64)
inline constexpr char kTorClientComponentId[] =
    "monolafkoghdlanndjfeebmdfkbklejg";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0xce, 0xde, 0xb0, 0x5a, 0xe6, 0x73, 0xb0, 0xdd, 0x39, 0x54, 0x41,
    0xc3, 0x5a, 0x1a, 0xb4, 0x96, 0x79, 0x29, 0x31, 0x93, 0x09, 0x9c,
    0xe0, 0x77, 0xb2, 0x76, 0x58, 0xe8, 0x51, 0x56, 0xfd, 0x32};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#else  // #if defined(ARCH_CPU_ARM64)
inline constexpr char kTorClientComponentId[] =
    "biahpgbdmdkfgndcmfiipgcebobojjkp";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0x18, 0x07, 0xf6, 0x13, 0xc3, 0xa5, 0x6d, 0x32, 0xc5, 0x88, 0xf6,
    0x24, 0x1e, 0x1e, 0x99, 0xaf, 0xb2, 0xde, 0x25, 0xc7, 0xd0, 0xbe,
    0x15, 0x86, 0xe8, 0xba, 0xa6, 0xc5, 0x36, 0x65, 0x70, 0x02};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#endif
#endif

// Returns the path for for where the Tor client binary is installed.
base::FilePath GetTorClientDirectory();

// Returns the path client execution path, based on the installation path for
// components, the `install_dir` provided, and the `filename`.
base::FilePath GetClientExecutablePath(const base::SafeBaseName& install_dir,
                                       const base::SafeBaseName& executable);

// Returns the path for the torrc file, based on the installation path for
// components, and the `install_dir` provided.
base::FilePath GetTorRcPath(const base::SafeBaseName& install_dir);

// Returns the path for the client's `--DataDirectory` argument.
base::FilePath GetTorDataPath();

// Return the directory path for the watcher arguments passed to the client.
base::FilePath GetTorWatchPath();

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_CONSTANTS_H_
