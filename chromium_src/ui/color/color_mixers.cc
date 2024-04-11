// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/color_mixers.h"

#include "ui/base/ui_base_features.h"

namespace {

bool GiveTrue() {
  return true;
}

}  // namespace

// AddMaterialUiColorMixer() should be called to get some colors(ex,
// kColorToastBackground) at specific order in the AddColorMixers().
// See the comments at ui::AddColorMixers().
#define IsChromeRefresh2023 IsChromeRefresh2023() || GiveTrue

#include "src/ui/color/color_mixers.cc"

#undef IsChromeRefresh2023
