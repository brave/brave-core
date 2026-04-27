// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_RESIZE_AREA_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_RESIZE_AREA_H_

#include "chrome/browser/ui/views/side_panel/side_panel_resize_area.h"
#include "ui/gfx/geometry/rect.h"

namespace views {

// Custom SidePanelResizeArea to adjust its position based on the presence
// of panel border.
class BraveSidePanelResizeArea : public SidePanelResizeArea {
  METADATA_HEADER(BraveSidePanelResizeArea, SidePanelResizeArea)

 public:
  using SidePanelResizeArea::SidePanelResizeArea;
  ~BraveSidePanelResizeArea() override;

  // Width of the resize strip when the panel has no border.
  static constexpr int kNoBorderResizeAreaWidth = 5;

  // Returns the resize area bounds for the no-border case.
  // `panel_on_right` is true when the panel appears on the right side of the
  // screen (right-aligned LTR or left-aligned RTL), which puts the resize
  // strip at the left edge so it faces the web content.
  gfx::Rect GetNoBorderResizeBounds(bool panel_on_right,
                                    const gfx::Rect& panel_bounds) const;

  // views::SidePanelResizeArea:
  void Layout(PassKey) override;
};

}  // namespace views

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_RESIZE_AREA_H_
