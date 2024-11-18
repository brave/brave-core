/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_ROUNDED_SEPARATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_ROUNDED_SEPARATOR_H_

#include <optional>
#include <string>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

// The RoundedSeparator class is a view that shows a line used to visually
// separate other views.
class RoundedSeparator : public views::View {
  METADATA_HEADER(RoundedSeparator, views::View)

 public:
  // The separator's thickness in dip.
  static constexpr int kThickness = 1;

  RoundedSeparator();
  RoundedSeparator(const RoundedSeparator&) = delete;
  RoundedSeparator& operator=(const RoundedSeparator&) = delete;
  ~RoundedSeparator() override;

  void SetColor(SkColor color);

  void SetPreferredHeight(int height);

  // Overridden from View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  int preferred_height_ = kThickness;
  std::optional<SkColor> overridden_color_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_ROUNDED_SEPARATOR_H_
