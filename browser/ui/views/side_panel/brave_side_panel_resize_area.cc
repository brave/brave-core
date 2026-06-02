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

gfx::Rect BraveSidePanelResizeArea::GetResizeStripBounds(
    bool panel_on_right,
    const gfx::Rect& panel_bounds) const {
  if (panel_on_right) {
    // Panel is on the right → strip sits at the left (content-facing) edge.
    return gfx::Rect(panel_bounds.x(), panel_bounds.y(), kResizeStripWidth,
                     panel_bounds.height());
  }
  // Panel is on the left → strip sits at the right (content-facing) edge.
  return gfx::Rect(panel_bounds.right() - kResizeStripWidth, panel_bounds.y(),
                   kResizeStripWidth, panel_bounds.height());
}

void BraveSidePanelResizeArea::Layout(PassKey) {
  // Upstream puts resize strip over the padding but we don't have enough
  // padding for it. Put resize strip over the local bound at the fixed
  // position.
  const bool panel_on_right =
      (side_panel_->IsRightAligned() && !base::i18n::IsRTL()) ||
      (!side_panel_->IsRightAligned() && base::i18n::IsRTL());
  SetBoundsRect(
      GetResizeStripBounds(panel_on_right, side_panel_->GetLocalBounds()));
}

BEGIN_METADATA(BraveSidePanelResizeArea)
END_METADATA

}  // namespace views
