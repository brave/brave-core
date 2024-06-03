/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/material_chrome_color_mixer.h"

#include "brave/browser/ui/color/material_brave_color_mixer.h"

#define AddMaterialChromeColorMixer AddMaterialChromeColorMixer_ChromiumImpl
#include "src/chrome/browser/ui/color/material_chrome_color_mixer.cc"
#undef AddMaterialChromeColorMixer

void AddMaterialChromeColorMixer(ui::ColorProvider* provider,
                                 const ui::ColorProviderKey& key) {
  AddMaterialChromeColorMixer_ChromiumImpl(provider, key);

#if !BUILDFLAG(IS_ANDROID)
  AddMaterialBraveColorMixer(provider, key);
#endif
}
