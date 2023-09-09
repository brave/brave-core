/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/omnibox_color_mixer.h"

#include "brave/browser/ui/color/brave_color_mixer.h"

#define AddOmniboxColorMixer AddOmniboxColorMixer_ChromiumImpl
#include "src/chrome/browser/ui/color/omnibox_color_mixer.cc"
#undef AddOmniboxColorMixer

namespace {

void AddBraveOmniboxColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key) {
  // Apply brave theme when there is no custom theme.
  if (key.custom_theme)
    return;
#if !BUILDFLAG(IS_ANDROID)
  key.color_mode == ui::ColorProviderKey::ColorMode::kDark
      ? AddBraveOmniboxDarkThemeColorMixer(provider, key)
      : AddBraveOmniboxLightThemeColorMixer(provider, key);
#endif  // #if !BUILDFLAG(IS_ANDROID)
}

}  // namespace

void AddOmniboxColorMixer(ui::ColorProvider* provider,
                          const ui::ColorProviderKey& key) {
  AddOmniboxColorMixer_ChromiumImpl(provider, key);
  AddBraveOmniboxColorMixer(provider, key);
}
