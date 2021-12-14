// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#define AddNativeUiColorMixer AddNativeUiColorMixer_Chromium
#include "src/ui/color/mac/native_color_mixers_mac.mm"
#undef AddNativeUiColorMixer

namespace ui {

void AddNativeUiColorMixer(ColorProvider* provider,
                           bool dark_window,
                           bool high_contrast) {
  if (@available(macOS 10.14, *)) {
    AddNativeUiColorMixer_Chromium(provider, dark_window, high_contrast);
    return;
  }

  if (high_contrast) {
    AddNativeUiColorMixer_Chromium(provider, dark_window, high_contrast);
    return;
  }

  // The rest of these overrides only targets for old macOS like high sierra
  // that doesn't support dark mode. We are using dark mode on old macOS but
  // some below colors are fetched from system color and they are not dark mode
  // aware. So, we should replace those colors with dark mode aware ui color.

  // Cache ui colors before upstream's native color is applied and then
  // re-apply.
  const SkColor color_menu_item_foreground =
      provider->GetColor(kColorMenuItemForeground);
  const SkColor color_menu_item_foreground_disabled =
      provider->GetColor(kColorMenuItemForegroundDisabled);
  const SkColor color_label_selection_background =
      provider->GetColor(kColorLabelSelectionBackground);
  const SkColor color_textfield_selection_background =
      provider->GetColor(kColorTextfieldSelectionBackground);

  AddNativeUiColorMixer_Chromium(provider, dark_window, high_contrast);

  ColorMixer& mixer = provider->AddMixer();
  mixer[kColorMenuItemForeground] = {color_menu_item_foreground};
  mixer[kColorMenuItemForegroundDisabled] = {
      color_menu_item_foreground_disabled};
  mixer[kColorLabelSelectionBackground] = {color_label_selection_background};
  mixer[kColorTextfieldSelectionBackground] = {
      color_textfield_selection_background};
}

}  // namespace ui
