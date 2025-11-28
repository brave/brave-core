// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/tabs/public/tab_collection.h"

#include "components/tabs/public/tab_interface.h"

// Store the old parent before updating new parent for OnReparentedImpl()
#define BRAVE_TAB_COLLECTION_ON_REPARENTED auto* old_parent = parent_.get();

// Calls OnReparentImpl() at the end of TabCollection::OnReparented() for child
// classes.
#define OnAncestorChanged(...)    \
  OnAncestorChanged(__VA_ARGS__); \
  }                               \
  {                               \
    OnReparentedImpl(old_parent, new_parent);

#include <components/tabs/impl/tab_collection.cc>

#undef OnAncestorChanged

namespace tabs {

void TabCollection::OnReparentedImpl(TabCollection* old_parent,
                                     TabCollection* new_parent) {}

}  // namespace tabs
