/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_MIXER_H_
#define BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_MIXER_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider_manager.h"

namespace ui {
class ColorProvider;
}  // namespace ui

// Exposed for testing.
SkColor GetLocationBarBackground(bool dark, bool priv, bool hover);
SkColor GetOmniboxResultBackground(int id, bool dark, bool priv);

void AddBraveThemeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderManager::Key& key);
void AddBravifiedChromeThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key);
void AddBravePrivateThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderManager::Key& key);
void AddBraveTorThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderManager::Key& key);
void AddBraveOmniboxLightThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key);
void AddBraveOmniboxDarkThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key);
void AddBraveOmniboxPrivateThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key);

#endif  // BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_MIXER_H_
