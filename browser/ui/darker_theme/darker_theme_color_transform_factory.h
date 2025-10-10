// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_DARKER_THEME_DARKER_THEME_COLOR_TRANSFORM_FACTORY_H_
#define BRAVE_BROWSER_UI_DARKER_THEME_DARKER_THEME_COLOR_TRANSFORM_FACTORY_H_

#include "ui/color/color_transform.h"

namespace ui {
struct ColorProviderKey;
}  // namespace ui

namespace darker_theme {

// Creates ui::ColorTransform that transform colors using given
// |reference_color_id|. This color transform should be applied only when the
// user has enabled the darker theme.
// When the user has chosen a custom theme color, the input color will be
// adjusted to have the sasme lightness as the reference color. Otherwise, the
// reference color will be used as is. Note that we're using `int` for
// |reference_color_id|'s type so that we can pass various color id types like
// ui::ColorId or nala::ColorId.
ui::ColorTransform ApplyDarknessFromColor(const ui::ColorProviderKey& key,
                                          int reference_color_id);

}  // namespace darker_theme

#endif  // BRAVE_BROWSER_UI_DARKER_THEME_DARKER_THEME_COLOR_TRANSFORM_FACTORY_H_
