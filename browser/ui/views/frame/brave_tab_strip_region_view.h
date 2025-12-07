/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_

#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

// `HorizontalTabStripRegionView` is customized to insert a medium-sized gap
// before the first tab when the browser frame is not fullscreen or maximized.
class BraveHorizontalTabStripRegionView : public HorizontalTabStripRegionView {
  METADATA_HEADER(BraveHorizontalTabStripRegionView,
                  HorizontalTabStripRegionView)

 public:
  template <typename... Args>
  explicit BraveHorizontalTabStripRegionView(Args&&... args)
      : HorizontalTabStripRegionView(std::forward<Args>(args)...) {
    Initialize();
  }

  ~BraveHorizontalTabStripRegionView() override;

  void Layout(PassKey) override;

 private:
  // TabStripRegionView:
  void UpdateTabStripMargin() override;
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;

  void Initialize();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_
