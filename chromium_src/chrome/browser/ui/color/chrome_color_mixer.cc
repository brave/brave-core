/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/chrome_color_mixer.h"

#include "brave/browser/ui/color/brave_color_mixer.h"

#define AddChromeColorMixer AddChromeColorMixer_ChromiumImpl
#include "src/chrome/browser/ui/color/chrome_color_mixer.cc"
#undef AddChromeColorMixer

namespace {

void AddBraveColorMixer(ui::ColorProvider* provider,
                        const ui::ColorProviderKey& key) {
#if !BUILDFLAG(IS_ANDROID)
  AddBraveThemeColorMixer(provider, key);
  AddBravifiedChromeThemeColorMixer(provider, key);
#endif  // #if !BUILDFLAG(IS_ANDROID)
}

}  // namespace

void AddChromeColorMixer(ui::ColorProvider* provider,
                         const ui::ColorProviderKey& key) {
  AddChromeColorMixer_ChromiumImpl(provider, key);
  AddBraveColorMixer(provider, key);
}
