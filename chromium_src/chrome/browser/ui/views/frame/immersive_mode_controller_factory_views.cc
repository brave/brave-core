/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#if defined(USE_AURA)
#include "brave/browser/ui/views/frame/immersive_mode_controller_aura.h"

#define CreateImmersiveModeController CreateImmersiveModeController_Unused
#endif

#include "src/chrome/browser/ui/views/frame/immersive_mode_controller_factory_views.cc"

#if defined(USE_AURA)
#undef CreateImmersiveModeController

namespace chrome {

std::unique_ptr<ImmersiveModeController> CreateImmersiveModeController(
    const BrowserView* browser_view) {
  return std::make_unique<ImmersiveModeControllerAura>();
}

}  // namespace chrome
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
