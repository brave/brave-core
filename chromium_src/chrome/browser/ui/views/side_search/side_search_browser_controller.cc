// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Include any corresponding headers first that define the same macros and
// therefore avoid undef before use.
#include "chrome/browser/ui/views/side_search/side_search_browser_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

#define SidePanel BraveSidePanel
#include "src/chrome/browser/ui/views/side_search/side_search_browser_controller.cc"
#undef SidePanel
