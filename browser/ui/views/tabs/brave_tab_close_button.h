/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CLOSE_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CLOSE_BUTTON_H_

#include <memory>

#include "chrome/browser/ui/views/tabs/tab_close_button.h"

class BraveTabCloseButton : public TabCloseButton {
 public:
  BraveTabCloseButton(views::ButtonListener* listener,
                      MouseEventCallback mouse_event_callback);
  ~BraveTabCloseButton() override = default;

  BraveTabCloseButton(const BraveTabCloseButton&) = delete;
  BraveTabCloseButton& operator=(const BraveTabCloseButton&) = delete;

 private:
  // TabCloseButton overrides:
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CLOSE_BUTTON_H_
