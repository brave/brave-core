/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_ROUNDED_SEPARATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_ROUNDED_SEPARATOR_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/view.h"

// The RoundedSeparator class is a view that shows a line used to visually
// separate other views.
class RoundedSeparator : public views::View {
 public:
  // The separator's class name.
  static const char kViewClassName[];

  // The separator's thickness in dip.
  static const int kThickness;

  RoundedSeparator();
  RoundedSeparator(const RoundedSeparator&) = delete;
  RoundedSeparator& operator=(const RoundedSeparator&) = delete;
  ~RoundedSeparator() override;

  void SetColor(SkColor color);

  void SetPreferredHeight(int height);

  // Overridden from View:
  gfx::Size CalculatePreferredSize() const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnPaint(gfx::Canvas* canvas) override;
  const char* GetClassName() const override;

 private:
  int preferred_height_ = kThickness;
  absl::optional<SkColor> overridden_color_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_ROUNDED_SEPARATOR_H_
