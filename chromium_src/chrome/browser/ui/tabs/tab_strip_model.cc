/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "components/tabs/public/tab_strip_collection.h"

#define DraggingTabsSession DraggingTabsSessionChromium

// Use Brave version of TabStripCollection.
#if !BUILDFLAG(IS_ANDROID)
#define TabStripCollection BraveTabStripCollection
#endif  // !BUILDFLAG(IS_ANDROID)

#include <chrome/browser/ui/tabs/tab_strip_model.cc>  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID)
#undef TabStripCollection
#endif  // !BUILDFLAG(IS_ANDROID)

#undef DraggingTabsSession

void TabStripModel::CloseSelectedTabsWithSplitView() {
  NOTREACHED();
}
