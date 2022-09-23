// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/resize_area_delegate.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserView;

// Replacement for chromium's SidePanel which defines a
// unique inset and border style compared to Brave
class BraveSidePanel : public views::View,
                       public views::ViewObserver,
                       public views::ResizeAreaDelegate {
 public:
  // Determines the side from which the side panel will appear.
  // LTR / RTL conversions are handled in
  // BrowserViewLayout::LayoutSidePanelView. As such, left will always be on the
  // left side of the browser regardless of LTR / RTL mode.
  enum HorizontalAlignment { kHorizontalAlignLeft = 0, kHorizontalAlignRight };

  METADATA_HEADER(BraveSidePanel);
  // Same signature as chromium SidePanel
  explicit BraveSidePanel(BrowserView* browser_view,
                          HorizontalAlignment horizontal_alignment =
                              HorizontalAlignment::kHorizontalAlignLeft);
  BraveSidePanel(const BraveSidePanel&) = delete;
  BraveSidePanel& operator=(const BraveSidePanel&) = delete;
  ~BraveSidePanel() override;

  void SetPanelWidth(int width);
  void SetHorizontalAlignment(HorizontalAlignment alignment);
  HorizontalAlignment GetHorizontalAlignment();
  bool IsRightAligned();

  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override;

 private:
  void UpdateVisibility();
  void UpdateBorder();

  // views::View:
  void ChildVisibilityChanged(View* child) override;
  void OnThemeChanged() override;

  // views::ViewObserver:
  void OnChildViewAdded(View* observed_view, View* child) override;
  void OnChildViewRemoved(View* observed_view, View* child) override;

  HorizontalAlignment horizontal_alighment_ = kHorizontalAlignLeft;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_H_
