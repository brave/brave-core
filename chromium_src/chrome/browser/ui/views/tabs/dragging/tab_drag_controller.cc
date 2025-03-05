/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"

#define TabDragController TabDragControllerChromium

// Remove drag threshold when it's vertical tab strip
#define GetHorizontalDragThreshold()                          \
  GetHorizontalDragThreshold() -                              \
      (tabs::utils::ShouldShowVerticalTabs(                   \
           BrowserView::GetBrowserViewForNativeWindow(        \
               GetAttachedBrowserWidget()->GetNativeWindow()) \
               ->browser())                                   \
           ? attached_context_->GetHorizontalDragThreshold()  \
           : 0)

#include "src/chrome/browser/ui/views/tabs/dragging/tab_drag_controller.cc"

#undef GetHorizontalDragThreshold
#undef GetBrowserViewForNativeWindow
#undef TabDragController
