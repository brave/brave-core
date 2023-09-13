/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/grit/generated_resources.h"

// Undef upstream's to avoid redefined error.
#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW

#define IDS_TOOLTIP_SIDE_PANEL_HIDE IDS_TOOLTIP_SIDEBAR_HIDE
#define IDS_TOOLTIP_SIDE_PANEL_SHOW IDS_TOOLTIP_SIDEBAR_SHOW

#define SidePanel BraveSidePanel
#include "src/chrome/browser/ui/views/side_panel/side_panel_coordinator.cc"
#undef SidePanel

#undef IDS_TOOLTIP_SIDE_PANEL_HIDE
#undef IDS_TOOLTIP_SIDE_PANEL_SHOW
