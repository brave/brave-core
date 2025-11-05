// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_H_

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

// This exclusion is added because chrome/browser/ui/webui/favicon_source.cc is
// custom added to the android build, which ends up pulling these dependecies
// that we dont build.
#if !BUILDFLAG(IS_ANDROID) || BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)

#include <extensions/common/manifest.h>  // IWYU pragma: export

#endif  // #if !BUILDFLAG(IS_ANDROID) || BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_H_
