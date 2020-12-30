/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"

class BraveWindowFrameGraphic;

class BraveOpaqueBrowserFrameView : public OpaqueBrowserFrameView {
 public:
  BraveOpaqueBrowserFrameView(BrowserFrame* frame,
                              BrowserView* browser_view,
                              OpaqueBrowserFrameViewLayout* layout);
  ~BraveOpaqueBrowserFrameView() override;

  BraveOpaqueBrowserFrameView(const BraveOpaqueBrowserFrameView&) = delete;
  BraveOpaqueBrowserFrameView& operator=(
      const BraveOpaqueBrowserFrameView&) = delete;

  // BraveOpaqueBrowserFrameView overrides:
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_
