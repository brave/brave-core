// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/gfx/color_palette.h"
#include "ui/views/style/typography.h"

// Comes after the above includes.
#include "chrome/browser/ui/views/chrome_typography_provider.h"

namespace {
  const SkColor kBraveGrey800 = SkColorSetRGB(0x3b, 0x3e, 0x4f);
}

#define ChromeTypographyProvider ChromeTypographyProvider_ChromiumImpl
#include "../../../../../../chrome/browser/ui/views/chrome_typography_provider.cc"
#undef ChromeTypographyProvider

SkColor ChromeTypographyProvider::GetColor(const views::View& view,
                                           int context,
                                           int style) const {
  // Harmony check duplicated from ChromiumImpl
  const ui::NativeTheme* native_theme = view.GetNativeTheme();
  DCHECK(native_theme);
  if (ShouldIgnoreHarmonySpec(*native_theme)) {
    return GetHarmonyTextColorForNonStandardNativeTheme(context, style,
                                                        *native_theme);
  }
  // Override button text colors
  if (context == views::style::CONTEXT_BUTTON_MD) {
    switch (style) {
      case views::style::STYLE_DIALOG_BUTTON_DEFAULT:
        return SK_ColorWHITE;
      case views::style::STYLE_DISABLED:
        // Keep chromium style for this state.
        break;
      default:
        return native_theme->ShouldUseDarkColors() ? SK_ColorWHITE
                                                   : kBraveGrey800;
    }
  }
  return ChromeTypographyProvider_ChromiumImpl::GetColor(view, context, style);
}
