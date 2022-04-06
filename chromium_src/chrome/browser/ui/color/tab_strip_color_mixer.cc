/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/tab_strip_color_mixer.h"

#define AddTabStripColorMixer AddTabStripColorMixer_ChromiumImpl
#include "src/chrome/browser/ui/color/tab_strip_color_mixer.cc"
#undef AddTabStripColorMixer

namespace {

const SkColor kLightToolbarIcon = SkColorSetRGB(0x42, 0x42, 0x42);

void AddBraveTabStripColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // Tab text colors.
  if (key.color_mode == ui::ColorProviderManager::ColorMode::kDark) {
    mixer[kColorTabForegroundInactiveFrameActive] = {SK_ColorWHITE};
    mixer[kColorTabForegroundInactiveFrameInactive] = {SK_ColorWHITE};
  } else {
    mixer[kColorTabForegroundInactiveFrameActive] = {kLightToolbarIcon};
    mixer[kColorTabForegroundInactiveFrameInactive] = {kLightToolbarIcon};
  }
}

}  // namespace

void AddTabStripColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderManager::Key& key) {
  AddTabStripColorMixer_ChromiumImpl(provider, key);
  AddBraveTabStripColorMixer(provider, key);
}
