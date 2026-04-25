// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_COLLECTION_NODE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_COLLECTION_NODE_H_

#include "components/tabs/public/tab_collection.h"

// Keep this TabCollectionNode::Type enum in sync with our additions to
// tabs::TabCollection::Type enum.
#define UNPINNED UNPINNED, TREE_NODE

#include <chrome/browser/ui/views/tabs/vertical/tab_collection_node.h>  // IWYU pragma: export
#undef UNPINNED

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_TAB_COLLECTION_NODE_H_
