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
                               const ui::ColorProviderManager::Key& key) {
  // Apply brave theme when there is no custom theme.
  if (key.custom_theme)
    return;

  key.color_mode == ui::ColorProviderManager::ColorMode::kDark
      ? AddBraveOmniboxDarkThemeColorMixer(provider, key)
      : AddBraveOmniboxLightThemeColorMixer(provider, key);
}

}  // namespace

void AddOmniboxColorMixer(ui::ColorProvider* provider,
                          const ui::ColorProviderManager::Key& key) {
  AddOmniboxColorMixer_ChromiumImpl(provider, key);
  AddBraveOmniboxColorMixer(provider, key);
}
