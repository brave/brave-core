/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/material_tab_strip_color_mixer.h"

#define AddMaterialTabStripColorMixer AddMaterialTabStripColorMixer_ChromiumImpl
#include "src/chrome/browser/ui/color/material_tab_strip_color_mixer.cc"
#undef AddMaterialTabStripColorMixer

void AddMaterialTabStripColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  // Upstream's material tab strip colors are currently not used.
}
