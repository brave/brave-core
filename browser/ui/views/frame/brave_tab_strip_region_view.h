/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_

#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

// `TabStripRegionView` is customized to insert a medium-sized gap before the
// first tab when the browser frame is not fullscreen or maximized.
class BraveTabStripRegionView : public TabStripRegionView {
  METADATA_HEADER(BraveTabStripRegionView, TabStripRegionView)

 public:
  using TabStripRegionView::TabStripRegionView;
  BraveTabStripRegionView(const BraveTabStripRegionView&) = delete;
  BraveTabStripRegionView& operator=(const BraveTabStripRegionView&) = delete;
  ~BraveTabStripRegionView() override;

  void Layout(PassKey) override;

 private:
  void UpdateBraveTabStripMargins();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_
