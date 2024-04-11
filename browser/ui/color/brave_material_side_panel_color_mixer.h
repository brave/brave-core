/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_BRAVE_MATERIAL_SIDE_PANEL_COLOR_MIXER_H_
#define BRAVE_BROWSER_UI_COLOR_BRAVE_MATERIAL_SIDE_PANEL_COLOR_MIXER_H_

#include "ui/color/color_provider_key.h"

namespace ui {
class ColorProvider;
}  // namespace ui

void AddBraveMaterialSidePanelColorMixer(ui::ColorProvider* provider,
                                         const ui::ColorProviderKey& key);

#endif  // BRAVE_BROWSER_UI_COLOR_BRAVE_MATERIAL_SIDE_PANEL_COLOR_MIXER_H_
