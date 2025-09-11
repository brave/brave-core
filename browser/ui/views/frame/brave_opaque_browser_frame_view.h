/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveWindowFrameGraphic;

class BraveOpaqueBrowserFrameView : public OpaqueBrowserFrameView {
  METADATA_HEADER(BraveOpaqueBrowserFrameView, OpaqueBrowserFrameView)
 public:
  BraveOpaqueBrowserFrameView(BrowserFrame* frame,
                              BrowserView* browser_view,
                              OpaqueBrowserFrameViewLayout* layout);
  ~BraveOpaqueBrowserFrameView() override;

  BraveOpaqueBrowserFrameView(const BraveOpaqueBrowserFrameView&) = delete;
  BraveOpaqueBrowserFrameView& operator=(
      const BraveOpaqueBrowserFrameView&) = delete;

  // OpaqueBrowserFrameView overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  int NonClientHitTest(const gfx::Point& point) override;
  void UpdateCaptionButtonPlaceholderContainerBackground() override;
  void PaintClientEdge(gfx::Canvas* canvas) const override;
  int GetTopInset(bool restored) const override;
  int GetTopAreaHeight() const override;

 private:
  bool ShouldShowVerticalTabs() const;
  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_
