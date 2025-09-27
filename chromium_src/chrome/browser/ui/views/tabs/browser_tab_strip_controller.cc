/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"

#include "chrome/browser/ui/tabs/split_tab_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

// To use original |model_index| value always instead of updating it from
// split_tabs::GetIndexOfLastActiveTab(). We want to activate clicked tab. But
// upstream activates the most recently focused tab in the split when selecting
// a split tab. If it's in split tab and another tab in same split can't be
// activated, follow upstream's behavior. Ex, when another tab in split is
// blocked tab with tab-modal, clicked tab can't be activated till that tab
// model is dismissed.
namespace split_tabs {
int BraveGetIndexOfLastActiveTab(TabStripModel* tab_strip_model,
                                 SplitTabId id,
                                 int model_index) {
  if (tab_strip_model->CanActivateTabAt(model_index)) {
    return model_index;
  }

  return GetIndexOfLastActiveTab(tab_strip_model, id);
}
}  // namespace split_tabs

#define GetIndexOfLastActiveTab(...) \
  BraveGetIndexOfLastActiveTab(__VA_ARGS__, model_index)

#include <chrome/browser/ui/views/tabs/browser_tab_strip_controller.cc>

#undef GetIndexOfLastActiveTab
