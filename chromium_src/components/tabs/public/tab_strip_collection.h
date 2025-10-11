// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_

// Overload to AddTabRecursive() with an additional parameter and make it
// virtual.
#define AddTabRecursive(...)                                  \
  virtual AddTabRecursive(__VA_ARGS__, TabInterface* opener); \
  void AddTabRecursive(__VA_ARGS__)

// Make methods virtual for BraveTabStripCollection.
#define MoveTabRecursive virtual MoveTabRecursive
#define MoveTabsRecursive virtual MoveTabsRecursive
#define RemoveTabAtIndexRecursive virtual RemoveTabAtIndexRecursive
#define RemoveTabRecursive virtual RemoveTabRecursive

#include <components/tabs/public/tab_strip_collection.h>  // IWYU pragma: export

#undef RemoveTabRecursive
#undef RemoveTabAtIndexRecursive
#undef MoveTabsRecursive
#undef MoveTabRecursive
#undef AddTabRecursive

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
