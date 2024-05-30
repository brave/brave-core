// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_GFX_COLOR_PALETTE_H_
#define BRAVE_CHROMIUM_SRC_UI_GFX_COLOR_PALETTE_H_

#include "src/ui/gfx/color_palette.h"  // IWYU pragma: export

namespace gfx {

// TODO(simonhong): Delete this file when nala color is available in ui layer.
// TODO(simonhong): Remove kBraveBlurple300. It is used incorrectly.
inline constexpr SkColor kBraveBlurple300 = SkColorSetRGB(0xA0, 0xA5, 0xEB);
inline constexpr SkColor kColorGray20 = SkColorSetRGB(0xDA, 0xDF, 0xE1);
inline constexpr SkColor kColorGray60 = SkColorSetRGB(0x3F, 0x4E, 0x55);
inline constexpr SkColor kBraveGrey800 = SkColorSetRGB(0x3b, 0x3e, 0x4f);
inline constexpr SkColor kColorButtonBackground =
    SkColorSetRGB(0x3F, 0x39, 0xE8);
inline constexpr SkColor kColorButtonDisabled =
    SkColorSetARGB(0x4D, 0x68, 0x7B, 0x85);
inline constexpr SkColor kColorTextInteractive =
    SkColorSetRGB(0x3F, 0x39, 0xE8);
inline constexpr SkColor kColorTextInteractiveDark =
    SkColorSetRGB(0x7C, 0x91, 0xFF);
inline constexpr SkColor kColorTextDisabled =
    SkColorSetARGB(0x80, 0x21, 0x27, 0x2A);
inline constexpr SkColor kColorTextDisabledDark =
    SkColorSetARGB(0x80, 0xEB, 0xEE, 0xF0);
inline constexpr SkColor kColorTextSecondary = SkColorSetRGB(0x3F, 0x4E, 0x55);
inline constexpr SkColor kColorTextSecondaryDark =
    SkColorSetRGB(0xDA, 0xDF, 0xE1);
inline constexpr SkColor kColorDividerInteractive =
    SkColorSetARGB(0x66, 0x54, 0x5F, 0xF8);

}  // namespace gfx

#endif  // BRAVE_CHROMIUM_SRC_UI_GFX_COLOR_PALETTE_H_
