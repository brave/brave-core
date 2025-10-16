/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_

#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/size.h"

namespace views {
class ButtonListener;
}

class BraveNewTabButton : public NewTabButton {
  // Note that NewTabButton is missing METADATA_HEADER, so we need to declare
  // TabStripControlButton as paren.
  METADATA_HEADER(BraveNewTabButton, TabStripControlButton)

 public:
  // This static members are shared with BraveTabSearchButton
  // TODO(sko) If we could make TabSearchButton inherit BraveNewTabButton,
  // we might not need to do this any more.
  static gfx::Size GetButtonSize();

  BraveNewTabButton(TabStripController* tab_strip_controller,
                    PressedCallback callback,
                    const gfx::VectorIcon& icon /* this won't be used */,
                    Edge fixed_flat_edge = Edge::kNone,
                    Edge animated_flat_edge = Edge::kNone,
                    BrowserWindowInterface* browser = nullptr);
  ~BraveNewTabButton() override;

 protected:
  // NewTabButton:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
