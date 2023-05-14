// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "cc/paint/paint_flags.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_utils.h"

namespace gfx {

gfx::Insets BraveAdjustVisualBorderForFont(const gfx::FontList& badge_font,
                                           const gfx::Insets& desired_insets) {
  return gfx::Insets::VH(3, 8);
}

}  // namespace gfx

// If we don't turn on antialiasing, the rounded corners on the label look
// terrible.
#define setColor      \
  setAntiAlias(true); \
  flags.setColor
#define AdjustVisualBorderForFont BraveAdjustVisualBorderForFont

#include "src/ui/views/badge_painter.cc"

#undef AdjustVisualBorderForFont
#undef setColor
