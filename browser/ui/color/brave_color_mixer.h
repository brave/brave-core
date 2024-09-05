/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_MIXER_H_
#define BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_MIXER_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider_key.h"

namespace ui {
class ColorProvider;
}  // namespace ui

// Handling normal profile's dark or light theme.
void AddBraveThemeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderKey& key);
void AddBravifiedChromeThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key);
void AddPrivateThemeColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key);
void AddTorThemeColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderKey& key);
void AddBraveOmniboxColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key);
void AddBraveOmniboxPrivateThemeColorMixer(ui::ColorProvider* provider,
                                           const ui::ColorProviderKey& key);
void AddBravifiedTabStripColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key);
#endif  // BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_MIXER_H_
