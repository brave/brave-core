/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)

#include "brave/grit/brave_theme_resources_map.h"

#define BRAVE_RESOURCES_UTIL                          \
  for (const auto& resource : kBraveThemeResources) { \
    storage.emplace_back(resource.path, resource.id); \
  }

#else
#define BRAVE_RESOURCES_UTIL
#endif  // !BUILDFLAG(IS_ANDROID)

#include "src/chrome/browser/resources_util.cc"
#undef BRAVE_RESOURCES_UTIL
