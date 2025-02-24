/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_

#include <memory>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

class ReaderModeToolbarView : public views::View {
  METADATA_HEADER(ReaderModeToolbarView, views::View)
 public:
  struct Delegate {
    virtual ~Delegate() = default;
    virtual void OnReaderModeToolbarActivate(ReaderModeToolbarView* toolbar) {}
  };

  ReaderModeToolbarView(content::BrowserContext* browser_context,
                        bool use_rounded_corners);
  ~ReaderModeToolbarView() override;

  ReaderModeToolbarView(const ReaderModeToolbarView&) = delete;
  ReaderModeToolbarView(ReaderModeToolbarView&&) = delete;
  ReaderModeToolbarView& operator=(const ReaderModeToolbarView&) = delete;
  ReaderModeToolbarView& operator=(ReaderModeToolbarView&&) = delete;

  void SetVisible(bool visible) override;

  content::WebContents* GetWebContentsForTesting();

  views::View* toolbar() const { return toolbar_.get(); }

  void SetDelegate(Delegate* delegate);
  void SwapToolbarContents(ReaderModeToolbarView* toolbar);
  void RestoreToolbarContents(ReaderModeToolbarView* toolbar);

  void ActivateContents();

 private:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

  bool use_rounded_corners_ = false;
  std::unique_ptr<views::WebView> toolbar_;
  std::unique_ptr<content::WebContents> toolbar_contents_;
  raw_ptr<Delegate> delegate_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
