// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"

// Comes after the above includes.
#include "chrome/browser/ui/views/chrome_typography_provider.h"

namespace {

// This function was removed from chrome_typography_provider.cc in
// https://chromium.googlesource.com/chromium/src/+/852890e because it pushed
// the Harmony colors to native theme. Trying to override colors there would be
// more inconvenient. Instead, restoring this function here so that we know when
// to fall onto Chromium code.
bool ShouldIgnoreHarmonySpec(const views::View& view) {
  const ui::NativeTheme* theme = view.GetNativeTheme();
  DCHECK(theme);
#if defined(OS_MAC)
  return false;
#else
  if (theme->UserHasContrastPreference())
    return true;
  if (theme->ShouldUseDarkColors())
    return false;

  // TODO(pbos): Revisit this check. Both GG900 and black are considered
  // "default black" as the common theme uses GG900 as primary color.
  const SkColor test_color =
      view.GetColorProvider()->GetColor(ui::kColorLabelForeground);
  const bool label_color_is_black =
      test_color == SK_ColorBLACK || test_color == gfx::kGoogleGrey900;
  return !label_color_is_black;
#endif  // defined(OS_MAC)
}

}  // namespace

#define ChromeTypographyProvider ChromeTypographyProvider_ChromiumImpl
#include "../../../../../../chrome/browser/ui/views/chrome_typography_provider.cc"
#undef ChromeTypographyProvider

SkColor ChromeTypographyProvider::GetColor(const views::View& view,
                                           int context,
                                           int style) const {
  if (ShouldIgnoreHarmonySpec(view)) {
    return ChromeTypographyProvider_ChromiumImpl::GetColor(view, context,
                                                           style);
  }

  // Override button text colors.
  if (context == views::style::CONTEXT_BUTTON_MD) {
    switch (style) {
      case views::style::STYLE_DIALOG_BUTTON_DEFAULT:
        return SK_ColorWHITE;
      case views::style::STYLE_DISABLED:
        // Keep chromium style for this state.
        break;
      default:
        // See GetColorId in typography_provider.cc for the order in which the
        // color is selected. This case matches style == style::STYLE_LINK and
        // context == style::CONTEXT_BUTTON_MD.
        const ui::NativeTheme* native_theme = view.GetNativeTheme();
        DCHECK(native_theme);
        return native_theme->ShouldUseDarkColors() ? SK_ColorWHITE
                                                   : gfx::kBraveGrey800;
    }
  }

  return ChromeTypographyProvider_ChromiumImpl::GetColor(view, context, style);
}
