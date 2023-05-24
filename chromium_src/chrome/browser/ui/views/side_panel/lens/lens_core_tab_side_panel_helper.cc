/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/views/frame/browser_view.h"
#endif

#define SidePanel BraveSidePanel
#include "src/chrome/browser/ui/views/side_panel/lens/lens_core_tab_side_panel_helper.cc"  // IWYU pragma: export
#undef SidePanel
