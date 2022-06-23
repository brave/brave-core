/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_COLOR_PALETTE_H_
#define BRAVE_BROWSER_UI_COLOR_COLOR_PALETTE_H_

#include "third_party/skia/include/core/SkColor.h"

// TODO(simonhong): Move all below colors to brave_color_mixer.cc.
// Originally this colors were defined in BraveThemeProperties but
// they are used from mixers also. When color fetching from theme
// provider is deprecated, only mixer will use these colors.
constexpr SkColor kLightToolbar = SkColorSetRGB(0xf3, 0xf3, 0xf3);
constexpr SkColor kLightFrame = SkColorSetRGB(0xd5, 0xd9, 0xdc);
constexpr SkColor kLightToolbarIcon = SkColorSetRGB(0x42, 0x42, 0x42);
constexpr SkColor kDarkOmniboxText = SkColorSetRGB(0xff, 0xff, 0xff);
constexpr SkColor kLightOmniboxText = SkColorSetRGB(0x42, 0x42, 0x42);
constexpr SkColor kDarkToolbar = SkColorSetRGB(0x30, 0x34, 0x43);
constexpr SkColor kDarkFrame = SkColorSetRGB(0x0C, 0x0C, 0x17);
constexpr SkColor kDarkToolbarIcon = SkColorSetRGB(0xed, 0xed, 0xed);
constexpr SkColor kPrivateFrame = SkColorSetRGB(0x19, 0x16, 0x2F);
constexpr SkColor kPrivateToolbar = SkColorSetRGB(0x32, 0x25, 0x60);
constexpr SkColor kPrivateTorFrame = SkColorSetRGB(0x19, 0x0E, 0x2A);
constexpr SkColor kPrivateTorToolbar = SkColorSetRGB(0x49, 0x2D, 0x58);

#endif  // BRAVE_BROWSER_UI_COLOR_COLOR_PALETTE_H_
