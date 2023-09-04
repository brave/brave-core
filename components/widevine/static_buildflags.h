/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Unlike build flags in the typical GN sense, this file hard-codes a flag.
// Doing it in this (non-dynamic) way prevents us from having to declare
// additional GN deps in our and upstream's code. We don't lose much because the
// flag defined here is not meant to be changed dynamically with `gn gen`. At
// the same time, we get the benefits associated with BUILDFLAG(...). And we
// reduce duplicate code by coding the conditions for when the flag should be
// activated only once, in this file. Finally, the introduction of the new
// identifier makes it very easy to search the code base for its occurrences,
// for the case when the flag becomes obsolete in the future and needs to be
// removed.

#ifndef BRAVE_COMPONENTS_WIDEVINE_STATIC_BUILDFLAGS_H_
#define BRAVE_COMPONENTS_WIDEVINE_STATIC_BUILDFLAGS_H_

#include "build/build_config.h"
#include "build/buildflag.h"  // IWYU pragma: export

#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_ARM64)
#define BUILDFLAG_INTERNAL_WIDEVINE_ARM64_DLL_FIX() (1)
#else
#define BUILDFLAG_INTERNAL_WIDEVINE_ARM64_DLL_FIX() (0)
#endif

#endif  // BRAVE_COMPONENTS_WIDEVINE_STATIC_BUILDFLAGS_H_
