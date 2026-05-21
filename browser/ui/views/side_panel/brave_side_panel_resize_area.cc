// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_area.h"

#include "base/i18n/rtl.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "ui/base/metadata/metadata_impl_macros.h"

namespace views {

BraveSidePanelResizeArea::~BraveSidePanelResizeArea() = default;

gfx::Rect BraveSidePanelResizeArea::GetNoBorderResizeBounds(
    bool panel_on_right,
    const gfx::Rect& panel_bounds) const {
  if (panel_on_right) {
    // Panel is on the right → resize strip sits at the left (inner) edge.
    return gfx::Rect(panel_bounds.x(), panel_bounds.y(),
                     kNoBorderResizeAreaWidth, panel_bounds.height());
  }
  // Panel is on the left → resize strip sits at the right (inner) edge.
  return gfx::Rect(panel_bounds.right() - kNoBorderResizeAreaWidth,
                   panel_bounds.y(), kNoBorderResizeAreaWidth,
                   panel_bounds.height());
}

void BraveSidePanelResizeArea::Layout(PassKey) {
  if (!side_panel_->GetInsets().IsEmpty()) {
    // Parent has a border: resize area sits in the border gap between the
    // panel edge and the content area. Delegate entirely to upstream.
    LayoutSuperclass<SidePanelResizeArea>(this);
    return;
  }

  // No border: content fills the full panel. Position this view as a
  // narrow strip at the inner edge of the panel so it overlaps the content
  // and can capture mouse events.
  const bool panel_on_right =
      (side_panel_->IsRightAligned() && !base::i18n::IsRTL()) ||
      (!side_panel_->IsRightAligned() && base::i18n::IsRTL());
  SetBoundsRect(
      GetNoBorderResizeBounds(panel_on_right, side_panel_->GetLocalBounds()));
}

BEGIN_METADATA(BraveSidePanelResizeArea)
END_METADATA

}  // namespace views
