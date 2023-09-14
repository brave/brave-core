/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/views/event_utils.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"

SidebarItemView::SidebarItemView(const std::u16string& accessible_name)
    : SidebarButtonView(accessible_name) {}

SidebarItemView::~SidebarItemView() = default;

void SidebarItemView::DrawHorizontalBorder(bool top) {
  DCHECK(!draw_horizontal_border_);

  draw_horizontal_border_ = true;
  draw_horizontal_border_top_ = top;
  SchedulePaint();
}

void SidebarItemView::ClearHorizontalBorder() {
  if (!draw_horizontal_border_) {
    return;
  }

  draw_horizontal_border_ = false;
  SchedulePaint();
}

void SidebarItemView::OnPaintBorder(gfx::Canvas* canvas) {
  ImageButton::OnPaintBorder(canvas);

  // Draw item highlight
  if (draw_highlight_) {
    auto& bundle = ui::ResourceBundle::GetSharedInstance();

    auto* image = bundle.GetImageSkiaNamed(
        draw_highlight_on_left_ ? IDR_SIDEBAR_ITEM_HIGHLIGHT
                                : IDR_SIDEBAR_ITEM_HIGHLIGHT_RIGHT);
    canvas->DrawImageInt(
        *image, draw_highlight_on_left_ ? 0 : width() - image->width(), 0);
  }

  const ui::ColorProvider* color_provider = GetColorProvider();
  if (draw_horizontal_border_ && color_provider) {
    constexpr float kHorizontalBorderWidth = 2;
    gfx::Rect border_rect(GetLocalBounds());

    if (!draw_horizontal_border_top_) {
      border_rect.set_y(border_rect.bottom() - kHorizontalBorderWidth);
    }

    border_rect.set_height(kHorizontalBorderWidth);

    canvas->FillRect(border_rect,
                     color_provider->GetColor(kColorSidebarItemDragIndicator));
  }
}

bool SidebarItemView::IsTriggerableEvent(const ui::Event& e) {
  return e.type() == ui::ET_GESTURE_TAP ||
         e.type() == ui::ET_GESTURE_TAP_DOWN ||
         event_utils::IsPossibleDispositionEvent(e);
}

BEGIN_METADATA(SidebarItemView, SidebarButtonView)
END_METADATA
