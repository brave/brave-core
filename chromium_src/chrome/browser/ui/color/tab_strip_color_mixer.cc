/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/tab_strip_color_mixer.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/color/brave_color_mixer.h"
#include "brave/browser/ui/tabs/brave_tab_color_mixer.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace {

void AddBraveTabStripColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
#if !BUILDFLAG(IS_ANDROID)
  AddBravifiedTabStripColorMixer(provider, key);
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace

#include <chrome/browser/ui/color/tab_strip_color_mixer.cc>
