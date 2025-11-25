/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Include the corresponding header first since it defines the same macros and
// therefore avoid undef before use.
#include "chrome/browser/ui/views/frame/browser_view.h"

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/views/brave_tab_search_bubble_host.h"
#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_impl_old.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"

#define InfoBarContainerView BraveInfoBarContainerView
#define BrowserViewLayout BraveBrowserViewLayout
#define ToolbarView BraveToolbarView
#define TabStripRegionView BraveTabStripRegionView
#define BookmarkBarView BraveBookmarkBarView
#define MultiContentsView BraveMultiContentsView

#define UpdateExclusiveAccessBubble(...)             \
  UpdateExclusiveAccessBubble(__VA_ARGS__) override; \
  virtual void UpdateExclusiveAccessBubble_ChromiumImpl(__VA_ARGS__)

#define BRAVE_BROWSER_VIEW_LAYOUT_CONVERTED_HIT_TEST \
  if (dst->GetWidget() != src->GetWidget()) {        \
    return false;                                    \
  }

#include <chrome/browser/ui/views/frame/browser_view.cc>

#undef UpdateExclusiveAccessBubble
#undef MultiContentsView
#undef BookmarkBarView
#undef TabStripRegionView
#undef ToolbarView
#undef BrowserViewLayout
#undef InfoBarContainerView
#undef BRAVE_BROWSER_VIEW_LAYOUT_CONVERTED_HIT_TEST

void BrowserView::SetNativeWindowPropertyForWidget(views::Widget* widget) {
  // Sets a kBrowserWindowKey to given child |widget| so that we can get
  // BrowserView from the |widget|.
  DCHECK(GetWidget());
  DCHECK_EQ(GetWidget(), widget->GetTopLevelWidget())
      << "The |widget| should be child of BrowserView's widget.";

  widget->SetNativeWindowProperty(kBrowserViewKey, this);
}

void BrowserView::ExclusiveAccessContextImpl::UpdateExclusiveAccessBubble(
    const ExclusiveAccessBubbleParams& params,
    ExclusiveAccessBubbleHideCallback first_hide_callback) {
  // Show/Hide full screen reminder bubble based on our settings preference
  // for tab-initiated ones.
  if (!GetProfile()->GetPrefs()->GetBoolean(kShowFullscreenReminder) &&
      params.type == EXCLUSIVE_ACCESS_BUBBLE_TYPE_FULLSCREEN_EXIT_INSTRUCTION) {
    return;
  }
  UpdateExclusiveAccessBubble_ChromiumImpl(params,
                                           std::move(first_hide_callback));
}
