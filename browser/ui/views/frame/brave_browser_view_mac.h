/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_MAC_H_

#include "ui/gfx/native_ui_types.h"

namespace brave {

// Sets the alpha value of the three macOS traffic-light buttons on `window`.
// `alpha` is clamped to [0, 1]. When alpha is 0, the buttons are also disabled
// so they cannot be clicked while invisible.
void SetTrafficLightsAlpha(gfx::NativeWindow window, double alpha);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_MAC_H_
