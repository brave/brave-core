/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"

#include <algorithm>

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/layout/layout_provider.h"

BraveTabSearchButton::~BraveTabSearchButton() = default;

gfx::Size BraveTabSearchButton::CalculatePreferredSize() const {
  return BraveNewTabButton::kButtonSize;
}

SkPath BraveTabSearchButton::GetBorderPath(const gfx::Point& origin,
                                           float scale,
                                           bool extend_to_top) const {
  if (disable_fill_color_)
    return {};

  return BraveNewTabButton::GetBorderPath(origin, scale, extend_to_top,
                                          GetCornerRadius(),
                                          GetContentsBounds().size());
}

int BraveTabSearchButton::GetCornerRadius() const {
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, GetContentsBounds().size());
}
