/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "brave/browser/ui/brave_browser_actions.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"
#include "brave/browser/ui/views/side_panel/bookmarks/brave_bookmarks_side_panel_coordinator.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"
#include "chrome/browser/contextual_tasks/contextual_tasks_side_panel_coordinator.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "chrome/browser/ui/views/side_panel/bookmarks/bookmarks_side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/history_clusters/history_clusters_side_panel_coordinator.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_browser_window_helper.h"
#endif

#define BrowserContentSettingBubbleModelDelegate \
  BraveBrowserContentSettingBubbleModelDelegate
#define BrowserLocationBarModelDelegate BraveLocationBarModelDelegate
#define BrowserWindowFeatures BrowserWindowFeatures_ChromiumImpl
#define SidePanelCoordinator BraveSidePanelCoordinator
#define BookmarksSidePanelCoordinator BraveBookmarksSidePanelCoordinator
#define BrowserActions BraveBrowserActions
#define BrowserCommandController BraveBrowserCommandController

#include <chrome/browser/ui/browser_window/internal/browser_window_features.cc>

#undef BookmarksSidePanelCoordinator
#undef SidePanelCoordinator
#undef BrowserWindowFeatures
#undef BrowserLocationBarModelDelegate
#undef BrowserContentSettingBubbleModelDelegate
#undef BrowserActions
#undef BrowserCommandController
