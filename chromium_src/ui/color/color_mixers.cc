// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ui/color/brave_ref_color_mixer.h"
#include "brave/ui/color/nala/nala_color_mixer.h"
#include "ui/color/ref_color_mixer.h"

#define AddRefColorMixer(...)         \
  AddRefColorMixer(__VA_ARGS__);      \
  AddBraveRefColorMixer(__VA_ARGS__); \
  nala::AddNalaColorMixer(__VA_ARGS__)

#include "src/ui/color/color_mixers.cc"

#undef AddRefColorMixer
