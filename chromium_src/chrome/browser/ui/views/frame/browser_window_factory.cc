/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#define BrowserFrame BraveBrowserFrame
#define BrowserView BraveBrowserView
#include "src/chrome/browser/ui/views/frame/browser_window_factory.cc"
#undef BrowserView
#undef BrowserFrame
