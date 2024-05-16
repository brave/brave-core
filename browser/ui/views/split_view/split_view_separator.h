/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/split_view/split_view_separator_delegate.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/controls/resize_area_delegate.h"

// A separator view that is located between contents web views in BrowserView.
// This separator is used to resize the contents web views.
class SplitViewSeparator : public views::ResizeArea,
                           public views::ResizeAreaDelegate {
  METADATA_HEADER(SplitViewSeparator, views::ResizeArea)
 public:
  SplitViewSeparator();
  ~SplitViewSeparator() override;

  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override;

  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override;

  void set_delegate(SplitViewSeparatorDelegate* delegate) {
    resize_area_delegate_ = delegate;
  }

 private:
  raw_ptr<SplitViewSeparatorDelegate> resize_area_delegate_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_H_
