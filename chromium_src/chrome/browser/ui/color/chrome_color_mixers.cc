/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/chrome_color_mixers.h"

namespace {

// There are some new color mixers that we don't need to call here.
// Replace them with this empty color.
// As call order of color mixer is important, we call some of them
// from where we want to. Just changing order could overwrite previous
// colors.
void EmptyColorMixer(ui::ColorProvider* provider,
                     const ui::ColorProviderKey& key) {}

}  // namespace

#include <chrome/browser/ui/color/chrome_color_mixers.cc>
