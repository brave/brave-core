// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// V2: compile upstream's SidePanelResizeArea implementation.
#include <chrome/browser/ui/views/side_panel/side_panel_resize_area.cc>
#else
// V1: excluded — Brave uses SidePanelResizeWidget instead.
#endif
