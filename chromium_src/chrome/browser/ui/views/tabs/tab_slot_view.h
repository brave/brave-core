// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TAB_SLOT_VIEW_H_
#define TAB_SLOT_VIEW_H_

#include "ui/views/view.h"

// Add a method to get information to nest tabs in a tree style.
#define SetGroup(...)                               \
  SetGroup_Unused() const {}                        \
  virtual TabNestingInfo GetTabNestingInfo() const; \
  virtual void SetGroup(__VA_ARGS__)

#include <chrome/browser/ui/views/tabs/tab_slot_view.h>

#undef SetGroup

#endif  // TAB_SLOT_VIEW_H_
