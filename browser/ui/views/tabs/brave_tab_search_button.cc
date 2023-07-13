/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"

#include <algorithm>
#include <memory>

#include "brave/browser/ui/views/brave_tab_search_bubble_host.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/layout/layout_provider.h"

BraveTabSearchButton::BraveTabSearchButton(TabStrip* tab_strip)
    : TabSearchButton(tab_strip) {
  tab_search_bubble_host_ = std::make_unique<BraveTabSearchBubbleHost>(
      this, tab_strip->controller()->GetProfile());
}

BraveTabSearchButton::~BraveTabSearchButton() = default;

gfx::Size BraveTabSearchButton::CalculatePreferredSize() const {
  return BraveNewTabButton::kButtonSize;
}

void BraveTabSearchButton::SetBubbleArrow(views::BubbleBorder::Arrow arrow) {
  static_cast<BraveTabSearchBubbleHost*>(tab_search_bubble_host_.get())
      ->SetBubbleArrow(arrow);
}

int BraveTabSearchButton::GetCornerRadius() {
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, GetContentsBounds().size());
}

BEGIN_METADATA(BraveTabSearchButton, TabSearchButton)
END_METADATA
