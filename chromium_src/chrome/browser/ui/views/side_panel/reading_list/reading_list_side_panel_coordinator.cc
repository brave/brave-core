// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"
#include "brave/browser/ui/views/side_panel/brave_read_later_side_panel_view.h"
#include "chrome/browser/ui/views/side_panel/reading_list/read_later_side_panel_web_view.h"

#if !BUILDFLAG(ENABLE_SIDEBAR_V2)
#define ReadLaterSidePanelWebView BraveReadLaterSidePanelView
#endif

#include <chrome/browser/ui/views/side_panel/reading_list/reading_list_side_panel_coordinator.cc>

#if !BUILDFLAG(ENABLE_SIDEBAR_V2)
#undef ReadLaterSidePanelWebView
#endif
