/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_BUBBLE_BORDER_WITH_ARROW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_BUBBLE_BORDER_WITH_ARROW_H_

#include "ui/views/bubble/bubble_border.h"

class BubbleBorderWithArrow : public views::BubbleBorder {
 public:
  static const int kBubbleArrowBoundsWidth = 12;
  static const int kBubbleArrowBoundsHeight = 18;

  static gfx::RectF GetArrowRect(
      const gfx::RectF& contents_bounds, views::BubbleBorder::Arrow arrow);

  using BubbleBorder::BubbleBorder;
  ~BubbleBorderWithArrow() override;

  BubbleBorderWithArrow(const BubbleBorderWithArrow&) = delete;
  BubbleBorderWithArrow& operator=(const BubbleBorderWithArrow&) = delete;

  // views::BubbleBorder overrides:
  gfx::Rect GetBounds(const gfx::Rect& anchor_rect,
                      const gfx::Size& contents_size) const override;
  void Paint(const views::View& view, gfx::Canvas* canvas) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_BUBBLE_BORDER_WITH_ARROWH_
