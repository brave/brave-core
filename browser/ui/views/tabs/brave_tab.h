/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_

#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/insets.h"

class TabController;

namespace gfx {
class AnimationContainer;
}

namespace chrome_browser_ui_views_tabs_tab_cc {
// Define methods that we want exposed from
// parent module's anonymous namespace
const gfx::RectF ScaleAndAlignBounds(const gfx::Rect& bounds,
                                      float scale,
                                      float stroke_thickness);
}


// Purposes for this child class:
// - Paint Brave-style Separators
// - Paint Shadows
// - Allow Tab to be inset more on top, so that it can draw shadows in
//   window area which is acting as non-client
class BraveTab : public Tab {
  public:
    BraveTab(TabController* controller, gfx::AnimationContainer* container);
    gfx::Insets GetContentsInsets() const override;
    void OnPaint(gfx::Canvas* canvas) override;
  private:
    // Paints the separator lines on the left and right edge of the tab if in
    // material refresh mode.
    void PaintSeparators(gfx::Canvas* canvas) override;
    void PaintTabShadows(gfx::Canvas* canvas, const gfx::Path& fill_path);
    void PaintTabBackgroundFill(gfx::Canvas* canvas,
                                 const gfx::Path& fill_path,
                                 bool active,
                                 bool paint_hover_effect,
                                 SkColor active_color,
                                 SkColor inactive_color,
                                 int fill_id,
                                 int y_inset) override;
    DISALLOW_COPY_AND_ASSIGN(BraveTab);
};

#endif
