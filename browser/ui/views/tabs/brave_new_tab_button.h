/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_

#include "chrome/browser/ui/views/tabs/new_tab_button.h"

class TabStrip;
namespace views {
  class ButtonListener;
}

class BraveNewTabButton : public NewTabButton {
  public:
    BraveNewTabButton(TabStrip* tab_strip, views::ButtonListener* listener);
    ~BraveNewTabButton() override;
  private:
    gfx::Size CalculatePreferredSize() const override;
    DISALLOW_COPY_AND_ASSIGN(BraveNewTabButton);
};

#endif
