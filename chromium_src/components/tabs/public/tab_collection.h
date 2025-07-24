// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_

#include "build/build_config.h"

// Add new collection Type enum value for Tree tab feature on Desktop.
#if !BUILDFLAG(IS_ANDROID)
#define SPLIT SPLIT, TREE_NODE
#endif  // !BUILDFLAG(IS_ANDROID)

#include <components/tabs/public/tab_collection.h>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef SPLIT
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TABS_PUBLIC_TAB_COLLECTION_H_
