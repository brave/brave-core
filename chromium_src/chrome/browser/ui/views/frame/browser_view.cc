/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"

#define ToolbarView BraveToolbarView
#define BrowserTabStripController BraveBrowserTabStripController
#include "../../../../../../../chrome/browser/ui/views/frame/browser_view.cc"  // NOLINT
#undef ToolbarView
#undef BrowserTabStripController
