/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace {

// Used to remove the drag threshold when it's a vertical tab strip: returns 0
// for a vertical tab strip, and 1 otherwise so nothing changes.
int VerticalTabsDragThresholdScale(TabDragContext& attached_context) {
  auto* vertical_tab_controller = VerticalTabController::From(
      BrowserView::GetBrowserViewForNativeWindow(
          attached_context.GetWidget()->GetNativeWindow())
          ->browser());
  return vertical_tab_controller &&
                 vertical_tab_controller->ShouldShowBraveVerticalTabs()
             ? 0
             : 1;
}

}  // namespace

#include <chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.cc>

base::PassKey<DraggingTabsSessionChromium>
DraggingTabsSessionChromium::GetPassKey() {
  return base::PassKey<DraggingTabsSessionChromium>();
}
