// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/chrome_typography_provider.h"
#include "ui/gfx/color_palette.h"

namespace gfx {
  const SkColor kBraveWhite = SkColorSetRGB(0xff, 0xff, 0xff);
  const SkColor kBraveGrey800 = SkColorSetRGB(0x3b, 0x3e, 0x4f);
}

// Button text color
#define kGoogleGrey900 kBraveWhite
// Button text color (prominent, dark mode)
#define kGoogleBlue300 kBraveWhite
// // Button text color (prominent, light mode)
#define kGoogleBlue600 kBraveGrey800
#include "../../../../../chrome/browser/ui/views/chrome_typography_provider.cc"
#undef kGoogleGrey900
#undef kGoogleBlue300
#undef kGoogleBlue600
