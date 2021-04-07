/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_theme_resources_map.h"

#define BRAVE_RESOURCES_UTIL                              \
  for (size_t i = 0; i < kBraveThemeResourcesSize; ++i) { \
    storage.emplace_back(kBraveThemeResources[i].path,    \
                         kBraveThemeResources[i].id);     \
  }

#include "../../../../chrome/browser/resources_util.cc"
#undef BRAVE_RESOURCES_UTIL
