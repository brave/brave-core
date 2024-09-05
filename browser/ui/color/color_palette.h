/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_COLOR_PALETTE_H_
#define BRAVE_BROWSER_UI_COLOR_COLOR_PALETTE_H_

#include "third_party/skia/include/core/SkColor.h"

// Although most of colors are only used at brave_color_mixer.cc,
// test also needs some colors. So, defined all here.
constexpr SkColor kPrivateColorForTest = SkColorSetRGB(0xFF, 0x00, 0x00);
constexpr SkColor kLightColorForTest = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kDarkColorForTest = SkColorSetRGB(0x00, 0x00, 0x00);

constexpr SkColor kPrivateFrame = SkColorSetRGB(0x13, 0x05, 0x2A);

constexpr SkColor kPrivateTorFrame = SkColorSetRGB(0x19, 0x04, 0x23);

constexpr SkColor kBraveNewTabBackgroundDark = SkColorSetRGB(0x33, 0x36, 0x39);
constexpr SkColor kBraveNewTabBackgroundLight = SkColorSetRGB(0x6B, 0x70, 0x84);

constexpr SkAlpha kBraveDisabledControlAlpha = 0x66;  // 40%

#endif  // BRAVE_BROWSER_UI_COLOR_COLOR_PALETTE_H_
