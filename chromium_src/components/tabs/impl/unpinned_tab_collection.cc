// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/tabs/public/tab_collection.h"

// Add TREE_NODE type to supported child collections in init list.
#if !BUILDFLAG(IS_ANDROID)
#define SPLIT SPLIT, TabCollection::Type::TREE_NODE
#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/impl/unpinned_tab_collection.cc>

#if !BUILDFLAG(IS_ANDROID)
#undef SPLIT
#endif  // !BUILDFLAG(IS_ANDROID)
