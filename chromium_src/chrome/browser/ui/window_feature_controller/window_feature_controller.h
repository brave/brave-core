/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WINDOW_FEATURE_CONTROLLER_WINDOW_FEATURE_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WINDOW_FEATURE_CONTROLLER_WINDOW_FEATURE_CONTROLLER_H_

#include <optional>

#include "build/build_config.h"

// Immersive mode requires special handling for vertical tabs, including
// tracking the startup state of vertical tabs mode
#if BUILDFLAG(IS_MAC)
#define UsesImmersiveFullscreenMode                            \
  UsesImmersiveFullscreenMode_ChromiumImpl() const;            \
  bool UsesImmersiveFullscreenTabbedMode_ChromiumImpl() const; \
                                                               \
 private:                                                      \
  mutable std::optional<bool> vertical_tabs_on_at_startup_;    \
                                                               \
 public:                                                       \
  bool UsesImmersiveFullscreenMode
#endif

#include <chrome/browser/ui/window_feature_controller/window_feature_controller.h>  // IWYU pragma: export

#if BUILDFLAG(IS_MAC)
#undef UsesImmersiveFullscreenMode
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WINDOW_FEATURE_CONTROLLER_WINDOW_FEATURE_CONTROLLER_H_
