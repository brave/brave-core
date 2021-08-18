// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/no_destructor.h"
// include this header so that setStyle redefine doesn't flow to cc::PaintFlags
#include "cc/paint/paint_flags.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/skia_util.h"

// include header first so that GetNativeTheme redefine doesn't flow to View
#include "ui/views/controls/focus_ring.h"

// Override the Focus Ring's color.
// In Chromium, this is specified via platform-specfic native theme,
// using kColorId_FocusedBorderColor. However, only macOS Light native theme
// overrides this. Since we do not have a Brave version of either
// platform-specific, or common versions, and we only want to override a single
// color, we use this micro-theme for the FocusRingView.
namespace {

class FocusRingTheme {
 public:
  SkColor GetSystemColor(int id) {
    // At the time of implementation, only two Color IDs were possible.
    // If this changes, consider overriding NativeTheme, or moving to
    // ThemeProperties.
    DCHECK(id == ui::NativeTheme::kColorId_FocusedBorderColor ||
            id == ui::NativeTheme::kColorId_AlertSeverityHigh);
    // Must be colors that are OK on dark or light bg since this is
    // a very simplistic implementation.
    switch (id) {
      case ui::NativeTheme::kColorId_FocusedBorderColor:
        return SkColorSetARGB(0x66, 0xfb, 0x54, 0x2b);
      case ui::NativeTheme::kColorId_AlertSeverityHigh:
        return SkColorSetRGB(0xf4, 0x34, 0x05);
    }
    return gfx::kPlaceholderColor;
  }
};

FocusRingTheme& GetFocusRingTheme() {
  static FocusRingTheme instance;
  return instance;
}

}  // namespace

#define setStyle                                                           \
  setColor(color_.value_or(                                                \
      GetFocusRingTheme().GetSystemColor(ColorIdForValidity(!invalid_)))); \
  paint.setStyle

#include "../../../../../ui/views/controls/focus_ring.cc"

#undef setStyle
