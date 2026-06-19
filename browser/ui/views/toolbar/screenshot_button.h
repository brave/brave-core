// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUTTON_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class BrowserWindowInterface;

namespace views {
class BubbleDialogModelHost;
}  // namespace views

class ScreenshotButton : public ToolbarButton {
  METADATA_HEADER(ScreenshotButton, ToolbarButton)
 public:
  explicit ScreenshotButton(BrowserWindowInterface* browser_window_interface);
  ScreenshotButton(const ScreenshotButton&) = delete;
  ScreenshotButton& operator=(const ScreenshotButton&) = delete;
  ~ScreenshotButton() override;

 private:
  void ButtonPressed();

  void OnBubbleClosing(views::Widget::ClosedReason reason);

  raw_ptr<BrowserWindowInterface> browser_window_interface_;

  std::unique_ptr<views::Widget> bubble_widget_;

  base::WeakPtrFactory<ScreenshotButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUTTON_H_
