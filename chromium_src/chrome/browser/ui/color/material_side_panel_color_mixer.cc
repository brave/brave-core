/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/material_side_panel_color_mixer.h"

#include "brave/browser/ui/color/brave_material_side_panel_color_mixer.h"

#define AddMaterialSidePanelColorMixer \
  AddMaterialSidePanelColorMixer_ChromiumImpl
#include "src/chrome/browser/ui/color/material_side_panel_color_mixer.cc"
#undef AddMaterialSidePanelColorMixer

void AddMaterialSidePanelColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  AddMaterialSidePanelColorMixer_ChromiumImpl(provider, key);
#if !BUILDFLAG(IS_ANDROID)
  AddBraveMaterialSidePanelColorMixer(provider, key);
#endif
}
