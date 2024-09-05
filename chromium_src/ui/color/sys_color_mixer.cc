// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/sys_color_mixer.h"

#include "brave/ui/color/brave_sys_color_mixer.h"

#define AddSysColorMixer AddSysColorMixer_Chromium

#include "src/ui/color/sys_color_mixer.cc"

#undef AddSysColorMixer

namespace ui {
void AddSysColorMixer(ColorProvider* provider, const ColorProviderKey& key) {
  AddSysColorMixer_Chromium(provider, key);
  ui::AddBraveSysColorMixer(provider, key);
}
}  // namespace ui
