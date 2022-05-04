/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_BUBBLE_BORDER_WITH_ARROW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_BUBBLE_BORDER_WITH_ARROW_H_

#include "ui/gfx/geometry/rect_f.h"
#include "ui/views/bubble/bubble_border.h"

class BubbleBorderWithArrow : public views::BubbleBorder {
 public:
  static const int kBubbleArrowBoundsWidth = 12;
  static const int kBubbleArrowBoundsHeight = 18;

  static gfx::RectF GetArrowRect(const gfx::RectF& contents_bounds,
                                 views::BubbleBorder::Arrow arrow);

  BubbleBorderWithArrow(Arrow arrow, Shadow shadow, SkColor color);
  ~BubbleBorderWithArrow() override;

  BubbleBorderWithArrow(const BubbleBorderWithArrow&) = delete;
  BubbleBorderWithArrow& operator=(const BubbleBorderWithArrow&) = delete;

  // views::BubbleBorder overrides:
  void Paint(const views::View& view, gfx::Canvas* canvas) override;
  SkRRect GetClientRect(const views::View& view) const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_BUBBLE_BORDER_WITH_ARROW_H_
