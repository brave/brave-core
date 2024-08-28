// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/sys_color_mixer.h"

#include "brave/ui/color/brave_sys_color_mixer.h"

#define kGrayscale kGrayscale) {                        \
    AddGrayscaleSysColorOverrides(mixer, key);          \
    ui::AddBraveGrayscaleSysColorOverrides(mixer, key); \
  }                                                     \
  else if (false

#include "src/ui/color/sys_color_mixer.cc"

#undef kGrayscale
