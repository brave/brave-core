// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/tab_strip_api/tab_strip_api.mojom.h"

// Add switch-case handling for TREE_NODE type in
// TabConverter::BuildMojoTabCollection()
#define kSplitTab                            \
  kSplitTab;                                 \
  break;                                     \
  case tabs::TabCollection::Type::TREE_NODE: \
    tab_collection->collection_type =        \
        tabs_api::mojom::TabCollection::CollectionType::kTreeNode

#include "src/chrome/browser/ui/tabs/tab_strip_api/converters/tab_converters.cc"

#undef kSplitTab
