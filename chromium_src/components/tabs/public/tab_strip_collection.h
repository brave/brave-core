// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_

// Creates a wrapper method to avoid crash due to Brave specific customization,
// especially in the case of tree tabs.
#define RemoveTabAtIndexRecursive(...)             \
  RemoveTabAtIndexRecursive_Chromium(__VA_ARGS__); \
  std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(__VA_ARGS__)

#include <components/tabs/public/tab_strip_collection.h>  // IWYU pragma: export

#undef RemoveTabAtIndexRecursive

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
