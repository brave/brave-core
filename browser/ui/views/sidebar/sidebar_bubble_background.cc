/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_bubble_background.h"

#include "brave/browser/ui/views/sidebar/bubble_border_with_arrow.h"
#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/views/view.h"

SidebarBubbleBackground::SidebarBubbleBackground(BubbleBorderWithArrow* border)
    : border_(border) {}

SidebarBubbleBackground::~SidebarBubbleBackground() = default;

void SidebarBubbleBackground::Paint(gfx::Canvas* canvas,
                                    views::View* view) const {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(border_->background_color());

  gfx::RectF bounds(view->GetLocalBounds());
  bounds.Inset(gfx::Insets(border_->GetInsets()));
  // Fill the background except arrow area in this bubble.
  bounds.Inset(gfx::Insets::TLBR(
      0, BubbleBorderWithArrow::kBubbleArrowBoundsWidth, 0, 0));
  canvas->DrawRoundRect(bounds, border_->corner_radius(), flags);
}
