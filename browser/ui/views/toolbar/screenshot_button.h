// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUTTON_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/widget/widget_observer.h"

class Browser;

namespace brave {
class ScreenshotController;
}  // namespace brave

class ScreenshotButton : public ToolbarButton, public views::WidgetObserver {
  METADATA_HEADER(ScreenshotButton, ToolbarButton)
 public:
  explicit ScreenshotButton(Browser* browser);
  ScreenshotButton(const ScreenshotButton&) = delete;
  ScreenshotButton& operator=(const ScreenshotButton&) = delete;
  ~ScreenshotButton() override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

 private:
  void ButtonPressed();

  raw_ptr<Browser> browser_;
  std::unique_ptr<brave::ScreenshotController> controller_;
  raw_ptr<views::Widget> bubble_widget_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUTTON_H_
