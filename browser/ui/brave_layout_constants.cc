/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "ui/base/material_design/material_design_controller.h"

int GetLayoutConstant(LayoutConstant constant) {
  const bool hybrid = ui::MaterialDesignController::GetMode() ==
                      ui::MaterialDesignController::MATERIAL_HYBRID;
  switch (constant) {
    case LOCATION_BAR_BUBBLE_VERTICAL_PADDING:
      return hybrid ? 1 : 3;
    case LOCATION_BAR_BUBBLE_FONT_VERTICAL_PADDING:
      return hybrid ? 3 : 2;
    case LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET:
      if (ui::MaterialDesignController::IsSecondaryUiMaterial())
        return 1;
      return hybrid ? 8 : 6;
    case LOCATION_BAR_ELEMENT_PADDING:
      return hybrid ? 3 : 1;
    case LOCATION_BAR_HEIGHT:
      return hybrid ? 32 : 28;
    case TABSTRIP_NEW_TAB_BUTTON_OVERLAP:
      // Originally here was `return hybrid ? 6 : 5`
      // This value means how the new tab button is moved to the most right tab
      // from the position of rectangle shape.
      // The was required to have beautiful positioning with inclided sides of
      // tabs and new tab button. Does not required for rectangle tabs and button.
      return hybrid ? 0 : 0;
    case TAB_HEIGHT:
      return hybrid ? 33 : 29;
    case TOOLBAR_ELEMENT_PADDING:
      return hybrid ? 8 : 0;
    case TOOLBAR_STANDARD_SPACING:
      return hybrid ? 8 : 4;
  }
  NOTREACHED();
  return 0;
}

gfx::Insets GetLayoutInsets(LayoutInset inset) {
  const bool hybrid = ui::MaterialDesignController::GetMode() ==
                      ui::MaterialDesignController::MATERIAL_HYBRID;
  switch (inset) {
    case TAB:
      // Originally here was `return gfx::Insets(1, hybrid ? 18 : 16)`
      // Insets is a content border inside of the tab. When the tab has been
      // changed to rectangle shape instead of trapezoid, then top part
      // of the tab had more space. So reasonable to decrease the content
      // border.
      return gfx::Insets(1, hybrid ? 9 : 8);
  }
  NOTREACHED();
  return gfx::Insets();
}

gfx::Size GetLayoutSize(LayoutSize size) {
  const bool hybrid = ui::MaterialDesignController::GetMode() ==
                      ui::MaterialDesignController::MATERIAL_HYBRID;
  switch (size) {
    case NEW_TAB_BUTTON:
      return hybrid ? gfx::Size(39, 21) : gfx::Size(36, 18);
  }
  NOTREACHED();
  return gfx::Size();
}
