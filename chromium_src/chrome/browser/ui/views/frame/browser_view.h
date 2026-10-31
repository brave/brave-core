/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/views/bookmarks/brave_bookmark_bar_view.h"
#include "build/build_config.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/views/frame/shadow_overlay_view.h"

#if BUILDFLAG(IS_WIN)
// <winuser.h> defines LoadAccelerators, which would rename
// BrowserView::LoadAccelerators() in translation units that include it.
#undef LoadAccelerators
#endif

#include <chrome/browser/ui/views/frame/browser_view.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
