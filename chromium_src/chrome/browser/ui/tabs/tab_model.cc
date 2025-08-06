// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/tabs/public/tab_collection.h"

// Add switch case handling for TREE_NODE type in TabModel::UpdateProperties().
#define UNPINNED \
  UNPINNED:      \
  case TabCollection::Type::TREE_NODE

#include <chrome/browser/ui/tabs/tab_model.cc>

#undef UNPINNED
