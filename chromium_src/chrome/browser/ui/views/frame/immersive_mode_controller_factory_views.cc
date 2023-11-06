/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/views/frame/immersive_mode_controller_win.h"

#define CreateImmersiveModeController CreateImmersiveModeController_Unused
#endif  // BUILDFLAG(IS_WIN)

#include "src/chrome/browser/ui/views/frame/immersive_mode_controller_factory_views.cc"

#if BUILDFLAG(IS_WIN)
#undef CreateImmersiveModeController

namespace chrome {

std::unique_ptr<ImmersiveModeController> CreateImmersiveModeController(
    const BrowserView* browser_view) {
  return std::make_unique<ImmersiveModeControllerWin>();
}

}  // namespace chrome
#endif  // BUILDFLAG(IS_WIN)
