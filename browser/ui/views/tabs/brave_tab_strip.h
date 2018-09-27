/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_

#include "chrome/browser/ui/views/tabs/tab_strip.h"

class BraveTabStrip : public TabStrip {
  public:
    using TabStrip::TabStrip;
    bool IsPositionInWindowCaption(const gfx::Point& point) override;
    // views::View
    void OnPaint(gfx::Canvas* canvas) override;
  private:
    void GenerateIdealBounds() override;
    DISALLOW_COPY_AND_ASSIGN(BraveTabStrip);
};

#endif