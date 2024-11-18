/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/rounded_separator.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/generated_resources.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/native_theme/native_theme.h"

// static
RoundedSeparator::RoundedSeparator() = default;

RoundedSeparator::~RoundedSeparator() = default;

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

gfx::Size RoundedSeparator::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size(kThickness, preferred_height_);
  gfx::Insets insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

void RoundedSeparator::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  // A valid role must be set in the AXNodeData prior to setting the name
  // via AXNodeData::SetName.
  node_data->role = ax::mojom::Role::kSplitter;
  node_data->SetName(l10n_util::GetStringUTF8(IDS_ACCNAME_SEPARATOR));
}

void RoundedSeparator::OnPaint(gfx::Canvas* canvas) {
  SkColor color = overridden_color_
                      ? *overridden_color_
                      : GetColorProvider()->GetColor(ui::kColorSeparator);

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

BEGIN_METADATA(RoundedSeparator)
END_METADATA
