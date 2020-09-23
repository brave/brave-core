/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#if defined(OS_WIN) || defined(OS_MAC)
#include "brave/browser/ui/views/frame/brave_browser_frame.h"

#define BrowserFrame BraveBrowserFrame
#endif

#define BrowserView BraveBrowserView
#include "../../../../../../../chrome/browser/ui/views/frame/browser_window_factory.cc"  // NOLINT
#undef BrowserView

#if defined(OS_WIN) || defined(OS_MAC)
#undef BrowserFrame
#endif
