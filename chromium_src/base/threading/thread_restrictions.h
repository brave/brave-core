/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_
#define BRAVE_CHROMIUM_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_

#include "brave/components/widevine/static_buildflags.h"

class BraveBrowsingDataRemoverDelegate;
namespace ipfs {
class IpfsService;
}
namespace brave {
class ProcessLauncher;
}

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
namespace component_updater {
class WidevineArm64DllInstaller;
}
#endif

#define BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H_BASE \
  friend class ::BraveBrowsingDataRemoverDelegate;     \
  friend class ipfs::IpfsService;                      \
  friend class brave::ProcessLauncher;

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
// WidevineArm64DllInstaller needs to use TimedWait:
#define BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_WIDEVINE_ARM64_DLL_FIX \
  friend class component_updater::WidevineArm64DllInstaller;
#else
#define BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_WIDEVINE_ARM64_DLL_FIX
#endif

#define BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H \
  BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H_BASE  \
  BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_WIDEVINE_ARM64_DLL_FIX

#include "src/base/threading/thread_restrictions.h"  // IWYU pragma: export
#undef BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H
#undef BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_H_BASE
#undef BRAVE_SCOPED_ALLOW_BASE_SYNC_PRIMITIVES_WIDEVINE_ARM64_DLL_FIX

#endif  // BRAVE_CHROMIUM_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_
