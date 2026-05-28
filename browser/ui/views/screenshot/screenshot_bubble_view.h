// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SCREENSHOT_SCREENSHOT_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SCREENSHOT_SCREENSHOT_BUBBLE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;

namespace content {
class WebContents;
}  // namespace content

namespace views {
class MdTextButton;
}  // namespace views

namespace brave {

class ScreenshotController;

class ScreenshotBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(ScreenshotBubbleView, views::BubbleDialogDelegateView)

 public:
  // Creates and shows the bubble anchored to `anchor`. Returns the bubble's
  // widget so the caller can track / close it.
  static views::Widget* Show(Browser* browser,
                             views::View* anchor,
                             ScreenshotController* controller);

  ScreenshotBubbleView(Browser* browser,
                       views::View* anchor,
                       ScreenshotController* controller);
  ScreenshotBubbleView(const ScreenshotBubbleView&) = delete;
  ScreenshotBubbleView& operator=(const ScreenshotBubbleView&) = delete;
  ~ScreenshotBubbleView() override;

 private:
  void OnVisibleAreaPressed();
  void OnFullPagePressed();
  content::WebContents* GetActiveWebContents();

  raw_ptr<Browser> browser_;
  raw_ptr<ScreenshotController> controller_;
  raw_ptr<views::MdTextButton> visible_area_button_ = nullptr;
  raw_ptr<views::MdTextButton> full_page_button_ = nullptr;

  base::WeakPtrFactory<ScreenshotBubbleView> weak_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_SCREENSHOT_SCREENSHOT_BUBBLE_VIEW_H_
