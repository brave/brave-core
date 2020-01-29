/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/rounded_separator.h"

#include <algorithm>

#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/canvas.h"
#include "ui/native_theme/native_theme.h"

// static
const char RoundedSeparator::kViewClassName[] = "RoundedSeparator";

// static
const int RoundedSeparator::kThickness = 1;

RoundedSeparator::RoundedSeparator() {}

RoundedSeparator::~RoundedSeparator() {}

void RoundedSeparator::SetColor(SkColor color) {
  overridden_color_ = color;
  SchedulePaint();
}

void RoundedSeparator::SetPreferredHeight(int height) {
  preferred_height_ = height;
  PreferredSizeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// Separator, View overrides:

gfx::Size RoundedSeparator::CalculatePreferredSize() const {
  gfx::Size size(kThickness, preferred_height_);
  gfx::Insets insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

void RoundedSeparator::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kSplitter;
}

void RoundedSeparator::OnPaint(gfx::Canvas* canvas) {
  SkColor color = overridden_color_
                      ? *overridden_color_
                      : GetNativeTheme()->GetSystemColor(
                            ui::NativeTheme::kColorId_SeparatorColor);

  float dsf = canvas->UndoDeviceScaleFactor();

  // The separator fills its bounds, but avoid filling partial pixels.
  gfx::Rect aligned = gfx::ScaleToEnclosedRect(GetContentsBounds(), dsf, dsf);


  // At least 1 pixel should be drawn to make the separator visible.
  aligned.set_width(std::max(1, aligned.width()));
  aligned.set_height(std::max(1, aligned.height()));

  const int separator_radius = aligned.width() / 2;

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(color);
  canvas->DrawRoundRect(aligned, separator_radius, flags);

  View::OnPaint(canvas);
}

const char* RoundedSeparator::GetClassName() const {
  return kViewClassName;
}
