/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_

#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/geometry/rect.h"

namespace chrome_browser_ui_views_tabs_tab_cc {

const gfx::RectF ScaleAndAlignBounds(const gfx::Rect& bounds,
                                     float scale,
                                     float stroke_thickness);

}

class BraveTab : public Tab {
  public:
    using Tab::Tab;
  private:
    // Paints the separator lines on the left and right edge of the tab if in
    // material refresh mode.
    void PaintSeparators(gfx::Canvas* canvas) override;
    DISALLOW_COPY_AND_ASSIGN(BraveTab);
};

#endif
