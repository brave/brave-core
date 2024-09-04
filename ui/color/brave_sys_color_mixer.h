// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_UI_COLOR_BRAVE_SYS_COLOR_MIXER_H_
#define BRAVE_UI_COLOR_BRAVE_SYS_COLOR_MIXER_H_

namespace ui {
class ColorProvider;
struct ColorProviderKey;

void AddBraveSysColorMixer(ColorProvider*, const ColorProviderKey& key);
}  // namespace ui

#endif  // BRAVE_UI_COLOR_BRAVE_SYS_COLOR_MIXER_H_
