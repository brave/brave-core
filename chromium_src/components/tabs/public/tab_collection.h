// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
// Add new collection Type enum value for Tree tab feature on Desktop.
#define SPLIT SPLIT, TREE_NODE

// Make static version of GetPassKey() and GetChildren() available for
// TreeTabNode to build/flatten tree tabs.
#define OnTabRemovedFromTree()                                                \
  OnTabRemovedFromTab_Unused() {}                                             \
                                                                              \
 protected:                                                                   \
  static base::PassKey<TabCollection> GetPassKeyStatic() {                    \
    return {};                                                                \
  }                                                                           \
  static const ChildrenVector& GetChildrenStatic(TabCollection& collection) { \
    return collection.GetChildren();                                          \
  }                                                                           \
                                                                              \
 public:                                                                      \
  void OnTabRemovedFromTree()

// Add OnReparentedImpl() method that can be overriden from TreeTabNode.
// This will be called from OnReparented().
#define OnCollectionAddedToTree(...)                       \
  OnCollectionAddedToTree(__VA_ARGS__);                    \
  virtual void OnReparentedImpl(TabCollection* old_parent, \
                                TabCollection* new_parent)

#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/public/tab_collection.h>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef OnCollectionAddedToTree
#undef OnTabRemovedFromTree
#undef SPLIT
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_
