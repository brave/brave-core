/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_

#include <memory>

#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"

namespace content {
class WebContents;
}

class Browser;
class ReaderModeToolbarView : public views::View, content::WebContentsDelegate {
  METADATA_HEADER(ReaderModeToolbarView, views::View)
 public:
  struct Delegate {
    virtual ~Delegate() = default;
    virtual void OnReaderModeToolbarActivate(ReaderModeToolbarView* toolbar) {}
  };

  explicit ReaderModeToolbarView(Browser* browser);
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

 private:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // WebContentsDelegate:
  void ActivateContents(content::WebContents* contents) override;

  std::unique_ptr<views::WebView> toolbar_;
  std::unique_ptr<content::WebContents> toolbar_contents_;
  raw_ptr<Delegate> delegate_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
