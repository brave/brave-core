/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"

#include "brave/browser/themes/theme_properties.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"

SidebarItemView::~SidebarItemView() = default;

void SidebarItemView::OnPaintBorder(gfx::Canvas* canvas) {
  ImageButton::OnPaintBorder(canvas);

  // Draw item highlight
  if (draw_highlight_) {
    auto& bundle = ui::ResourceBundle::GetSharedInstance();
    canvas->DrawImageInt(*bundle.GetImageSkiaNamed(IDR_SIDEBAR_ITEM_HIGHLIGHT),
                         0, 0);
  }
}

void SidebarItemView::OnPaintBackground(gfx::Canvas* canvas) {
  SidebarButtonView::OnPaintBackground(canvas);

  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    if (paint_background_on_hovered_ && GetState() == STATE_HOVERED) {
      canvas->FillRect(
          GetLocalBounds(),
          theme_provider->GetColor(
              BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND));
    }
  }
}
