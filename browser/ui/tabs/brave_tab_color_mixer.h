/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_COLOR_MIXER_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_COLOR_MIXER_H_

#include "ui/color/color_provider_key.h"

namespace tabs {

// Called from
// //brave/chromium_src/chrome/browser/ui/color/tab_strip_color_mixer.cc
void AddBraveTabThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key);

// Called from
// //brave/browser/themes/brave_private_window_theme_supplier.cc
void AddBraveTabPrivateThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key);
void AddBraveTabTorThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key);

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_COLOR_MIXER_H_
