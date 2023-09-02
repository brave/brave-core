/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_ROOT_VIEW_H_
#define BRAVE_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_ROOT_VIEW_H_

#include "brave/browser/ui/views/frame/vertical_tab_strip_root_view.h"

#include "chrome/browser/ui/views/frame/browser_root_view.h"

class VerticalTabStripRootView : public BrowserRootView {
 public:
  METADATA_HEADER(VerticalTabStripRootView);

  VerticalTabStripRootView(BrowserView* browser_view, views::Widget* widget);

  ~VerticalTabStripRootView() override;

  // BrowserRootView:
  bool OnMousePressed(const ui::MouseEvent& event) override;

  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;

  void OnMouseExited(const ui::MouseEvent& event) override;

 protected:
  void PaintChildren(const views::PaintInfo& paint_info) override;
};

#endif  // BRAVE_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_ROOT_VIEW_H_
