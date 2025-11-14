/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_SHIELDS_PAGE_INFO_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_SHIELDS_PAGE_INFO_VIEW_H_

#include <memory>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

class BrowserWindowInterface;
class WebUIContentsWrapper;

namespace content {
class WebContents;
}

namespace views {
class WebView;
}

// A view that displays Brave Shields settings within the page info bubble.
class BraveShieldsPageInfoView : public views::View {
  METADATA_HEADER(BraveShieldsPageInfoView, views::View)

 public:
  BraveShieldsPageInfoView(BrowserWindowInterface* browser_window_interface,
                           base::RepeatingCallback<void()> close_bubble);
  BraveShieldsPageInfoView(const BraveShieldsPageInfoView&) = delete;
  BraveShieldsPageInfoView& operator=(const BraveShieldsPageInfoView&) = delete;
  ~BraveShieldsPageInfoView() override;

  // Returns a value indicating whether this view should be displayed for the
  // specified web contents.
  static bool ShouldShowForWebContents(content::WebContents* web_contents);

  // Displays a Shields UI after repeated page reloads have been detected.
  void ShowRepeatedReloadsView();

  // views::View:
  void ChildPreferredSizeChanged(View* child) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  std::unique_ptr<WebUIContentsWrapper> CreateContentsWrapper();

  raw_ptr<BrowserWindowInterface> browser_ = nullptr;
  raw_ptr<views::WebView> web_view_ = nullptr;
  std::unique_ptr<WebUIContentsWrapper> contents_wrapper_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_SHIELDS_PAGE_INFO_VIEW_H_
