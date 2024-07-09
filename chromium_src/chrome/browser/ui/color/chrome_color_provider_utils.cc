/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/chrome_color_provider_utils.h"

#define ShouldApplyChromeMaterialOverrides \
  ShouldApplyChromeMaterialOverrides_UnUsed

#include "src/chrome/browser/ui/color/chrome_color_provider_utils.cc"

#undef ShouldApplyChromeMaterialOverrides

bool ShouldApplyChromeMaterialOverrides(const ui::ColorProviderKey& key) {
  return false;
}
