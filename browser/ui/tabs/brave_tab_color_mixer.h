/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_COLOR_MIXER_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_COLOR_MIXER_H_

#include "ui/color/color_provider_key.h"

namespace tabs {

void AddBraveTabLightThemeColorMixer(ui::ColorProvider* provider,
                                     const ui::ColorProviderKey& key);
void AddBraveTabDarkThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key);

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_COLOR_MIXER_H_
