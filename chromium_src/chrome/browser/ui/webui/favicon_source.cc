// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(IS_ANDROID) && !BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)

#ifndef IDR_DEFAULT_FAVICON_32
#define IDR_DEFAULT_FAVICON_32 IDR_DEFAULT_FAVICON
#endif
#ifndef IDR_DEFAULT_FAVICON_64
#define IDR_DEFAULT_FAVICON_64 IDR_DEFAULT_FAVICON
#endif
#ifndef IDR_DEFAULT_FAVICON_DARK_32
#define IDR_DEFAULT_FAVICON_DARK_32 IDR_DEFAULT_FAVICON_DARK
#endif
#ifndef IDR_DEFAULT_FAVICON_DARK_64
#define IDR_DEFAULT_FAVICON_DARK_64 IDR_DEFAULT_FAVICON_DARK
#endif

#endif  // #if BUILDFLAG(IS_ANDROID) && !BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)

#include <chrome/browser/ui/webui/favicon_source.cc>

#if BUILDFLAG(IS_ANDROID) && !BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)
#ifdef IDR_DEFAULT_FAVICON_DARK_64
#undef IDR_DEFAULT_FAVICON_DARK_64
#endif
#ifdef IDR_DEFAULT_FAVICON_DARK_32
#undef IDR_DEFAULT_FAVICON_DARK_32
#endif
#ifdef IDR_DEFAULT_FAVICON_64
#undef IDR_DEFAULT_FAVICON_64
#endif
#ifdef IDR_DEFAULT_FAVICON_32
#undef IDR_DEFAULT_FAVICON_32
#endif
#endif  // #if BUILDFLAG(IS_ANDROID) && !BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)
