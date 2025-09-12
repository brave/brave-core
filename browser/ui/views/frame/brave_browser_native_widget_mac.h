/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_NATIVE_WIDGET_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_NATIVE_WIDGET_MAC_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/views/frame/browser_native_widget_mac.h"

class BrowserView;

class BraveBrowserNativeWidgetMac : public BrowserNativeWidgetMac {
 public:
  BraveBrowserNativeWidgetMac(BrowserFrame* browser_frame,
                              BrowserView* browser_view);
  ~BraveBrowserNativeWidgetMac() override;

  // BrowserNativeWidgetMac:
  void GetWindowFrameTitlebarHeight(bool* override_titlebar_height,
                                    float* titlebar_height) override;
  void ValidateUserInterfaceItem(
      int32_t command,
      remote_cocoa::mojom::ValidateUserInterfaceItemResult* result) override;

  bool ExecuteCommand(int32_t command,
                      WindowOpenDisposition window_open_disposition,
                      bool is_before_first_responder) override;

 private:
  base::WeakPtr<BrowserView> browser_view_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_NATIVE_WIDGET_MAC_H_
