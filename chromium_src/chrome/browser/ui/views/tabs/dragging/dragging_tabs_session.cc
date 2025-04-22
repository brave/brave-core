/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#define DraggingTabsSession DraggingTabsSessionChromium

// Remove drag threshold when it's vertical tab strip
#define GetHorizontalDragThreshold()                         \
  GetHorizontalDragThreshold() -                             \
      (tabs::utils::ShouldShowVerticalTabs(                  \
           BrowserView::GetBrowserViewForNativeWindow(       \
               attached_context_->GetWidget()                \
                   ->GetTopLevelWidget()                     \
                   ->GetNativeWindow())                      \
               ->browser())                                  \
           ? attached_context_->GetHorizontalDragThreshold() \
           : 0)

#include "src/chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.cc"

#undef GetHorizontalDragThreshold
#undef DraggingTabsSession
