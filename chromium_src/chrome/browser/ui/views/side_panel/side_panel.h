// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// V2: use upstream's SidePanel class directly.
// Angle brackets are required here to resolve to the upstream file rather than
// this chromium_src override (which would cause infinite recursion).
#include <chrome/browser/ui/views/side_panel/side_panel.h>  // IWYU pragma: export
#else
// V1: use Brave's custom SidePanel replacement, which also prevents
// `chrome/browser/ui/views/side_panel/side_panel.cc` from being built.
#include "brave/browser/ui/views/side_panel/side_panel.h"  // IWYU pragma: export
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
