// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/native_theme/native_theme.h"

// Override the Focus Ring's color.
// In Chromium, this is specified via platform-specfic native theme,
// using kColorId_FocusedBorderColor. However, only macOS Light native theme
// overrides this. Since we do not have a Brave version of either
// platform-specific, or common versions, and we only want to override a single
// color, we use this micro-theme for the FocusRingView.
namespace {

ui::ColorId ColorIdForValidity(bool valid) {
  return valid ? ui::kColorFocusableBorderFocused : ui::kColorAlertHighSeverity;
}

class FocusRingTheme {
 public:
  SkColor GetSystemColor(::ui::ColorId color_id) {
    // At the time of implementation, only two Color IDs were possible.
    // If this changes, consider overriding NativeTheme, or moving to
    // ThemeProperties.
    DCHECK(color_id == ui::kColorFocusableBorderFocused ||
           color_id == ui::kColorAlertHighSeverity);
    // Must be colors that are OK on dark or light bg since this is
    // a very simplistic implementation.
    switch (color_id) {
      case ui::kColorFocusableBorderFocused:
        return SkColorSetARGB(0x66, 0xfb, 0x54, 0x2b);
      case ui::kColorAlertHighSeverity:
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

#include "../../../../../ui/views/controls/focus_ring.cc"

namespace views {

SkColor FocusRing::GetColor(View* focus_ring, bool valid) {
  // To avoid unused GetColor() in anonymous ns error.
  if (true) {
    return GetFocusRingTheme().GetSystemColor(ColorIdForValidity(valid));
  } else {
    return ::views::GetColor(focus_ring, valid);
  }
}

}  // namespace views
