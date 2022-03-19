// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#define AddNativeUiColorMixer AddNativeUiColorMixer_Chromium
#include "src/ui/color/mac/native_color_mixers_mac.mm"
#undef AddNativeUiColorMixer

namespace ui {

void AddNativeUiColorMixer(ColorProvider* provider,
                           const ColorProviderManager::Key& key) {
  if (@available(macOS 10.14, *)) {
    AddNativeUiColorMixer_Chromium(provider, key);
    return;
  }

  const bool high_contrast =
      key.contrast_mode == ColorProviderManager::ContrastMode::kHigh;
  if (high_contrast) {
    AddNativeUiColorMixer_Chromium(provider, key);
    return;
  }

  // The rest of these overrides only targets for old macOS like high sierra
  // that doesn't support dark mode. We are using dark mode on old macOS but
  // some below colors are fetched from system color and they are not dark mode
  // aware. So, we should replace those colors with dark mode aware ui color.
  AddNativeUiColorMixer_Chromium(provider, key);
  ColorMixer& mixer = provider->AddMixer();
  mixer[kColorMenuItemForeground] = {kColorPrimaryForeground};
  mixer[kColorMenuItemForegroundDisabled] = {kColorDisabledForeground};
}

}  // namespace ui
