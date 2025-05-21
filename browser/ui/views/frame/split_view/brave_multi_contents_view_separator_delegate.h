/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_SEPARATOR_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_SEPARATOR_DELEGATE_H_

// Use different delegate for BraveMultiContentsView.
// Although we have SplitViewSeparatorDelegate that inherits
// views::ResizeAreaDelegate but BraveMultiContentsView can't use it because it
// already inherits views::ResizeAreaDelegate.
class BraveMultiContentsViewSeparatorDelegate {
 public:
  virtual void OnSeparatorResize(int resize_amount, bool done_resizing) = 0;
  virtual void OnSeparatorDoubleClicked() = 0;

 protected:
  virtual ~BraveMultiContentsViewSeparatorDelegate() = default;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_SEPARATOR_DELEGATE_H_
