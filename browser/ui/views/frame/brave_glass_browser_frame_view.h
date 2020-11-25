/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_GLASS_BROWSER_FRAME_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_GLASS_BROWSER_FRAME_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/frame/glass_browser_frame_view.h"

class BraveWindowFrameGraphic;

class BraveGlassBrowserFrameView : public GlassBrowserFrameView {
 public:
  BraveGlassBrowserFrameView(BrowserFrame* frame, BrowserView* browser_view);
  ~BraveGlassBrowserFrameView() override;

  BraveGlassBrowserFrameView(const BraveGlassBrowserFrameView&) = delete;
  BraveGlassBrowserFrameView& operator=(
      const BraveGlassBrowserFrameView&) = delete;

 private:
  // GlassBrowserFrameView overrides:
  void OnPaint(gfx::Canvas* canvas) override;

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_GLASS_BROWSER_FRAME_VIEW_H_
