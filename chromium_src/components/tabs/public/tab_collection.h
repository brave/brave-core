// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
// Add new collection Type enum value for Tree tab feature on Desktop.
// Keep our additions to this tabs::TabCollection::Type enum in sync with
// TabCollectionNode::Type enum.
#define UNPINNED UNPINNED, TREE_NODE

// Make static version of GetPassKey() and GetChildren() available for
// TreeTabNode to build/flatten tree tabs.
#define OnTabRemovedFromTree()                             \
  OnTabRemovedFromTab_Unused() {}                          \
                                                           \
 protected:                                                \
  static base::PassKey<TabCollection> GetPassKeyStatic() { \
    return {};                                             \
  }                                                        \
  static const ChildrenVector& GetChildrenStatic(          \
      const TabCollection& collection) {                   \
    return collection.GetChildren();                       \
  }                                                        \
                                                           \
 public:                                                   \
  void OnTabRemovedFromTree()

#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/public/tab_collection.h>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef OnTabRemovedFromTree
#undef SPLIT
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_
