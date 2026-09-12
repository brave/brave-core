/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"
#include "chrome/browser/ui/tabs/split_tab_util.h"

namespace split_tabs {
int PassThrough(int model_index) {
  return model_index;
}
}  // namespace split_tabs

// To use original |model_index| value always instead of updating it from
// split_tabs::GetIndexOfLastActiveTab(). We want to activate clicked tab. But
// upstream activates the most recently focused tab in the split when selecting
// a split tab.
#define GetIndexOfLastActiveTab(...) PassThrough(model_index)

#define TabMenuModel BraveTabMenuModel

#include <chrome/browser/ui/views/tabs/browser_tab_strip_controller.cc>

#undef TabMenuModel
#undef GetIndexOfLastActiveTab
