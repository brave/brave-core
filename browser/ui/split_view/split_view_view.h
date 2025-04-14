/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_VIEW_H_
#define BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_VIEW_H_

// View interface for split view module.
class SplitViewView {
 public:
  virtual void Update() = 0;

 protected:
  ~SplitViewView() = default;
};

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_VIEW_H_
