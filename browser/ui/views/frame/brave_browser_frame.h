/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"

class BraveBrowserFrame : public BrowserFrame {
 public:
  explicit BraveBrowserFrame(BrowserView* browser_view);
  BraveBrowserFrame(const BraveBrowserFrame&) = delete;
  BraveBrowserFrame& operator=(const BraveBrowserFrame&) = delete;

  // BrowserFrame overrides:
  const ui::NativeTheme* GetNativeTheme() const override;

 private:
  raw_ptr<BrowserView> view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
