// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/tab_strip_api/adapters/tab_strip_model_adapter_impl.h"

#include "components/tabs/public/tab_collection.h"

// Add TREE_NODE type to supported child collections in init list.
#define SPLIT \
  SPLIT:      \
  case tabs::TabCollection::Type::TREE_NODE

#include <chrome/browser/ui/tabs/tab_strip_api/adapters/tab_strip_model_adapter_impl.cc>

#undef SPLIT
