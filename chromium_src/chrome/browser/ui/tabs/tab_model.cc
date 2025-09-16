/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_model.h"

#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "components/tabs/public/tab_collection.h"

// Add switch case handling for TREE_NODE type in TabModel::UpdateProperties().
#define UNPINNED \
  UNPINNED:      \
  case TabCollection::Type::TREE_NODE

#include <chrome/browser/ui/tabs/tab_model.cc>  // IWYU pragma: export

#undef UNPINNED

namespace tabs {

TabInterface* TabModel::GetOpener() {
  return opener_;
}
const TabInterface* TabModel::GetOpener() const {
  return opener_;
}

}  // namespace tabs
