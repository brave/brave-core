/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#define DraggingTabsSession DraggingTabsSessionChromium

// Remove the drag threshold when it's a vertical tab strip (we do this by
// multiplying by 0; if it's not a vertical tab strip, we multiply by 1 so
// nothing changes)
#define GetStandardWidth(...)                          \
  GetStandardWidth(__VA_ARGS__) *                      \
      (tabs::utils::ShouldShowVerticalTabs(            \
           BrowserView::GetBrowserViewForNativeWindow( \
               attached_context_->GetWidget()          \
                   ->GetTopLevelWidget()               \
                   ->GetNativeWindow())                \
               ->browser())                            \
           ? 0                                         \
           : 1)

#include "src/chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.cc"

#undef GetStandardWidth
#undef DraggingTabsSession
