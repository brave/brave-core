// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "build/android_buildflags.h"
#include "build/build_config.h"
#include "ui/resources/grit/ui_resources.h"

// Only used where favicon_source.cc's own SendDefaultResponse() references
// the 32/64 sizes: desktop and desktop Android.
#if !BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_DESKTOP_ANDROID)
// ui_resources.grd ships the larger favicons only on some platforms.
#if !defined(IDR_DEFAULT_FAVICON_32)
constexpr int BRAVE_IDR_DEFAULT_FAVICON_32 = IDR_DEFAULT_FAVICON;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_64 = IDR_DEFAULT_FAVICON;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_32 = IDR_DEFAULT_FAVICON_DARK;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_64 = IDR_DEFAULT_FAVICON_DARK;
#else
constexpr int BRAVE_IDR_DEFAULT_FAVICON_32 = IDR_DEFAULT_FAVICON_32;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_64 = IDR_DEFAULT_FAVICON_64;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_32 = IDR_DEFAULT_FAVICON_DARK_32;
constexpr int BRAVE_IDR_DEFAULT_FAVICON_DARK_64 = IDR_DEFAULT_FAVICON_DARK_64;
#endif  // !defined(IDR_DEFAULT_FAVICON_32)
#endif  // !BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_DESKTOP_ANDROID)

#include <chrome/browser/ui/webui/favicon_source.cc>
