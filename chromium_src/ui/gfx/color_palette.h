// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_GFX_COLOR_PALETTE_H_
#define BRAVE_CHROMIUM_SRC_UI_GFX_COLOR_PALETTE_H_

#include "src/ui/gfx/color_palette.h"  // IWYU pragma: export

namespace gfx {

// TODO(simonhong): Remove this. kBraveBlurple300 is used incorrectly.
inline constexpr SkColor kBraveBlurple300 = SkColorSetRGB(0xA0, 0xA5, 0xEB);
inline constexpr SkColor kBraveGrey800 = SkColorSetRGB(0x3b, 0x3e, 0x4f);
inline constexpr SkColor kBraveColorBrand = SkColorSetRGB(0x3F, 0x39, 0xE8);
inline constexpr SkColor kBraveColorBrandDark = SkColorSetRGB(0x7C, 0x91, 0xFF);

}  // namespace gfx

#endif  // BRAVE_CHROMIUM_SRC_UI_GFX_COLOR_PALETTE_H_
