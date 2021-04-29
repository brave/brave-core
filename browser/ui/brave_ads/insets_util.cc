/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/insets_util.h"

#include "build/build_config.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"

namespace brave_ads {

void AdjustInsetsForFontList(gfx::Insets* insets,
                             const gfx::FontList& font_list) {
#if defined(OS_WIN)
  // On Windows, the fonts can have slightly different metrics reported,
  // depending on where the code runs. In Chrome, DirectWrite is on, which means
  // font metrics are reported from Skia, which rounds from float using ceil.
  // In unit tests, however, GDI is used to report metrics, and the height
  // reported there is consistent with other platforms. This means there is a
  // difference of 1px in height between Chrome on Windows and everything else
  // (where everything else includes unit tests on Windows). This 1px causes the
  // text and everything else to stop aligning correctly, so we account for it
  // by shrinking the top padding by 1
  if (font_list.GetHeight() != 15) {
    *insets -= gfx::Insets(/* top */ 1, 0, 0, 0);
  }
#endif
}

}  // namespace brave_ads
