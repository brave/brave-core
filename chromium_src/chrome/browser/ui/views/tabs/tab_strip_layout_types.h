// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TAB_STRIP_LAYOUT_TYPES_H_
#define TAB_STRIP_LAYOUT_TYPES_H_

// A struct to represent tab nesting information for tree style tabs.
struct TabNestingInfo {
  int tree_height = 0;
  int level = 0;
};

#include <chrome/browser/ui/views/tabs/tab_strip_layout_types.h>

#endif  // TAB_STRIP_LAYOUT_TYPES_H_
