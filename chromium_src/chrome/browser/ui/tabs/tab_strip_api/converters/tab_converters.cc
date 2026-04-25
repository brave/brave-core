// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/browser_apis/tab_strip/tab_strip_api.mojom.h"

// For tree node, just return the unpinned tabs collection data. At the moment,
// this api is not used anyway.
#define SPLIT                                                                 \
  TREE_NODE: {                                                                \
    auto mojo_container = tabs_api::mojom::UnpinnedTabs::New();               \
    mojo_container->id = node_id;                                             \
    return tabs_api::mojom::Data::NewUnpinnedTabs(std::move(mojo_container)); \
  }                                                                           \
  case tabs::TabCollection::Type::SPLIT

#include <chrome/browser/ui/tabs/tab_strip_api/converters/tab_converters.cc>

#undef SPLIT
