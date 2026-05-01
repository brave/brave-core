/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_LINUX_NATIVE_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_LINUX_NATIVE_H_

#include <memory>

#include "chrome/browser/ui/views/frame/browser_frame_view_linux_native.h"

class BraveBrowserFrameViewLinuxNative : public BrowserFrameViewLinuxNative {
  METADATA_HEADER(BraveBrowserFrameViewLinuxNative, BrowserFrameViewLinuxNative)
 public:
  BraveBrowserFrameViewLinuxNative(
      BrowserWidget* frame,
      BrowserView* browser_view,
      BrowserFrameViewLayoutLinuxNative* layout,
      std::unique_ptr<ui::NavButtonProvider> nav_button_provider);
  ~BraveBrowserFrameViewLinuxNative() override;

  // BrowserFrameViewLinuxNative:
  void PaintRestoredFrameBorder(gfx::Canvas* canvas) const override;
  void MaybeUpdateCachedFrameButtonImages() override;
  void Layout(PassKey) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_LINUX_NATIVE_H_
