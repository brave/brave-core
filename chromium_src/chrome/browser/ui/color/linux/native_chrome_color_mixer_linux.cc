/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/native_chrome_color_mixer.h"

#define AddNativeChromeColorMixer AddNativeChromeColorMixer_ChromiumImpl
#include <chrome/browser/ui/color/linux/native_chrome_color_mixer_linux.cc>
#undef AddNativeChromeColorMixer

// We set `kColorOmniboxIconBackground` to transparent earlier in the mixer
// chain (see `AddBraveOmniboxColorMixer`), so the upstream
// `KeepBackgroundIfReadable()` check above runs against that transparent value
// instead of the real toolbar color, and always takes the tint fallback. Redo
// the check here against `kColorToolbar` directly: a readable toolbar is kept
// (visually equivalent to transparent, since the chip sits on the toolbar), and
// the tint is only used when genuinely unreadable.
void AddNativeChromeColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key) {
  AddNativeChromeColorMixer_ChromiumImpl(provider, key);
  if (key.system_theme == ui::SystemTheme::kDefault) {
    return;
  }

  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorOmniboxIconBackground] = ui::ColorTransform(base::BindRepeating(
      [](ui::ColorTransform fallback, SkColor /*input_color*/,
         const ui::ColorMixer& mixer) {
        const SkColor toolbar = mixer.GetResultColor(kColorToolbar);
        const SkColor text = mixer.GetResultColor(kColorOmniboxText);
        return color_utils::GetContrastRatio(text, toolbar) >=
                       color_utils::kMinimumReadableContrastRatio
                   ? toolbar
                   : fallback.Run(toolbar, mixer);
      },
      ui::AlphaBlend(kColorOmniboxText, kColorLocationBarBackground,
                     /*alpha=*/0x14)));
  mixer[kColorOmniboxIconForeground] =
      KeepForegroundIfReadable(kColorOmniboxIconBackground);
}
