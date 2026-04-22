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
    const gfx::Rect& parent_bounds) const {
  if (panel_on_right) {
    // Panel is on the right → resize strip sits at the left (inner) edge.
    return gfx::Rect(parent_bounds.x(), parent_bounds.y(),
                     kNoBorderResizeAreaWidth, parent_bounds.height());
  }
  // Panel is on the left → resize strip sits at the right (inner) edge.
  return gfx::Rect(parent_bounds.right() - kNoBorderResizeAreaWidth,
                   parent_bounds.y(), kNoBorderResizeAreaWidth,
                   parent_bounds.height());
}

void BraveSidePanelResizeArea::Layout(PassKey) {
  gfx::Rect local_bounds = parent()->GetLocalBounds();
  gfx::Rect contents_bounds = parent()->GetContentsBounds();

  if (local_bounds != contents_bounds) {
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
  SetBoundsRect(GetNoBorderResizeBounds(panel_on_right, local_bounds));
}

BEGIN_METADATA(BraveSidePanelResizeArea)
END_METADATA

}  // namespace views
