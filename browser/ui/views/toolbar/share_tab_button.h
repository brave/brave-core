/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_SHARE_TAB_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_SHARE_TAB_BUTTON_H_

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

namespace share_tab_button {
class ShareTabButton : public ToolbarButton {
 public:
  explicit ShareTabButton(PressedCallback callback);
  ShareTabButton(const ShareTabButton&) = delete;
  ShareTabButton& operator=(const ShareTabButton&) = delete;
  ~ShareTabButton() override;

  void UpdateImageAndText();
  
  // ToolbarButton:
  const char* GetClassName() const override;
};
}  // namespace share_tab_button

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_SHARE_TAB_BUTTON_H_