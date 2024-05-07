/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_MAC_H_

#include "chrome/browser/ui/views/frame/browser_frame_mac.h"

class Browser;

class BraveBrowserFrameMac : public BrowserFrameMac {
 public:
  BraveBrowserFrameMac(BrowserFrame* browser_frame, BrowserView* browser_view);
  ~BraveBrowserFrameMac() override;

  // BrowserFrameMac:
  void GetWindowFrameTitlebarHeight(bool* override_titlebar_height,
                                    float* titlebar_height) override;
  void ValidateUserInterfaceItem(
      int32_t command,
      remote_cocoa::mojom::ValidateUserInterfaceItemResult* result) override;

  bool ExecuteCommand(int32_t command,
                      WindowOpenDisposition window_open_disposition,
                      bool is_before_first_responder) override;

 private:
  raw_ptr<Browser> browser_;
  raw_ptr<BrowserView> browser_view_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_MAC_H_
