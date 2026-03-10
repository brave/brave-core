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

// Creates a wrapper method to avoid crash due to Brave specific customization,
// especially in the case of tree tabs.
#define RemoveTabAtIndexRecursive(...)             \
  RemoveTabAtIndexRecursive_Chromium(__VA_ARGS__); \
  virtual std::unique_ptr<TabInterface> RemoveTabAtIndexRecursive(__VA_ARGS__)

// Make methods virtual for BraveTabStripCollection.
#define MoveTabRecursive virtual MoveTabRecursive
#define MoveTabsRecursive virtual MoveTabsRecursive
#define CreateSplit virtual CreateSplit

// Make method virtual and make BraveTabStripCollection friend.
#define AddCollectionMapping(...)       \
  AddCollectionMapping_Unused();        \
  friend class BraveTabStripCollection; \
  virtual void AddCollectionMapping(__VA_ARGS__)
#define RemoveCollectionMapping virtual RemoveCollectionMapping

// Add public method for delegate to insert a collection at a position (e.g.
// when creating a split from tabs in different tree nodes). Inject in the
// public section by expanding the single occurrence of InsertTabCollectionAt
// (avoids redefining 'private', which would break included base headers).
// BraveTabStripCollection wraps this with a PassKey-restricted overload.
#define InsertTabCollectionAt                                           \
  AddTabCollectionAtPosition(std::unique_ptr<TabCollection> collection, \
                             const TabCollection::Position& position);  \
  void InsertTabCollectionAt

#include <components/tabs/public/tab_strip_collection.h>  // IWYU pragma: export

#undef InsertTabCollectionAt
#undef RemoveCollectionMapping
#undef AddCollectionMapping
#undef CreateSplit
#undef MoveTabsRecursive
#undef MoveTabRecursive
#undef AddTabRecursive
#undef RemoveTabAtIndexRecursive
#undef AddTabRecursive

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_STRIP_COLLECTION_H_
