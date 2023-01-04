/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Include the corresponding header first since it defines the same macros and
// therefore avoid undef before use.
#include "chrome/browser/ui/views/frame/browser_view.h"

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/grit/generated_resources.h"

// Undef upstream's to avoid redefined error.
#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW

#define InfoBarContainerView BraveInfoBarContainerView
#define BrowserViewLayout BraveBrowserViewLayout
#define ToolbarView BraveToolbarView
#define BrowserTabStripController BraveBrowserTabStripController
#define TabStrip BraveTabStrip
#define SidePanel BraveSidePanel
#define kAlignLeft kHorizontalAlignLeft
#define kAlignRight kHorizontalAlignRight
#define IDS_TOOLTIP_SIDE_PANEL_HIDE IDS_TOOLTIP_SIDEBAR_HIDE
#define IDS_TOOLTIP_SIDE_PANEL_SHOW IDS_TOOLTIP_SIDEBAR_SHOW

#include "src/chrome/browser/ui/views/frame/browser_view.cc"

#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW
#undef ToolbarView
#undef BrowserTabStripController
#undef TabStrip
#undef BrowserViewLayout
#undef SidePanel
#undef kAlignLeft
#undef kAlignRight
#undef InfoBarContainerView

// static
const char* BrowserView::GetBrowserViewKeyForNativeWindowProperty() {
  return kBrowserViewKey;
}
