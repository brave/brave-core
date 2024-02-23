/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_IOS)

#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia.h"

#define UI_GFX_PAINT_VECTOR_ICON_H_

namespace gfx {

// iOS can't include paint_vector_icon as it requires use_blink = true, but the
// bitmap generation code only requires this functionality to draw the passkey
// icon which is unused on iOS anyway, so we stub this functionality out below.

struct VectorIcon {
  VectorIcon() = default;
  VectorIcon(const VectorIcon&) = delete;
  VectorIcon& operator=(const VectorIcon&) = delete;
};

struct IconDescription {
  IconDescription(const gfx::VectorIcon& icon,
                  int dip_size = 0,
                  SkColor color = gfx::kPlaceholderColor,
                  const gfx::VectorIcon* badge_icon = nullptr) {}
  IconDescription(const IconDescription& other) = delete;
  ~IconDescription() = default;
};

ImageSkia CreateVectorIcon(const IconDescription& params) {
  return ImageSkia();
}

}  // namespace gfx

#endif

#include "src/components/qr_code_generator/bitmap_generator.cc"

#if BUILDFLAG(IS_IOS)
#undef UI_GFX_PAINT_VECTOR_ICON_H_
#endif
