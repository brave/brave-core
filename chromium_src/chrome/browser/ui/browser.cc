/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_browser_actions.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_tab_strip_model_delegate.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/bookmark/brave_bookmark_tab_helper.h"
#endif

#define BRAVE_BROWSER_CREATE return new BraveBrowser(params);
#define BrowserContentSettingBubbleModelDelegate \
  BraveBrowserContentSettingBubbleModelDelegate
#define BrowserCommandController BraveBrowserCommandController
#define BrowserLocationBarModelDelegate BraveLocationBarModelDelegate
#if !BUILDFLAG(IS_ANDROID)
#define BookmarkTabHelper BraveBookmarkTabHelper
#endif
#define BrowserTabStripModelDelegate BraveTabStripModelDelegate
#define BrowserActions(...) BraveBrowserActions(__VA_ARGS__)

#include "src/chrome/browser/ui/browser.cc"

#undef BrowserActions
#undef BrowserTabStripModelDelegate
#undef BrowserLocationBarModelDelegate
#undef BrowserContentSettingBubbleModelDelegate
#undef BrowserCommandController
#undef BRAVE_BROWSER_CREATE

#if !BUILDFLAG(IS_ANDROID)
#undef BookmarkTabHelper
#endif
