/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_DELEGATE_H_

#include "ui/views/controls/resize_area_delegate.h"

class SplitViewSeparatorDelegate : public views::ResizeAreaDelegate {
 public:
  virtual void OnDoubleClicked() = 0;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_DELEGATE_H_
