// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COLOR_BRAVE_DARKER_THEME_COLOR_TRANSFORM_FACTORY_H_
#define BRAVE_BROWSER_UI_COLOR_BRAVE_DARKER_THEME_COLOR_TRANSFORM_FACTORY_H_

#include "ui/color/color_transform.h"

namespace color {

// Creates ui::ColorTransform that uses given |transform| only when
// the user has enabled the darker theme. Otherwise, the returned ColorTransform
// will return the input color as it was passed.
ui::ColorTransform CreateBraveDarkerThemeColorTransform(
    const ui::ColorTransform& transform_for_darker_theme);

}  // namespace color

#endif  // BRAVE_BROWSER_UI_COLOR_BRAVE_DARKER_THEME_COLOR_TRANSFORM_FACTORY_H_
