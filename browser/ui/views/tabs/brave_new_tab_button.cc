/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/gfx/geometry/size.h"

namespace {
  // Returns the size of the button without any margin
  gfx::Size GetButtonSize() {
    return gfx::Size(20, 20);
  }
}

BraveNewTabButton::BraveNewTabButton(
                          TabStrip* tab_strip, views::ButtonListener* listener)
      : NewTabButton(tab_strip, listener) {
  // Overriden so that we use Brave's custom button size
  const int margin_vertical = (GetLayoutConstant(TAB_HEIGHT) -
                                   GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP) -
                                   GetButtonSize().height()) / 2;
  SetBorder(
    views::CreateEmptyBorder(gfx::Insets(margin_vertical, 8, 0, 8))
  );
}

BraveNewTabButton::~BraveNewTabButton() {}

gfx::Size BraveNewTabButton::CalculatePreferredSize() const {
  // Overriden so that we use Brave's custom button size
  gfx::Size size = GetButtonSize();
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

void BraveNewTabButton::PaintPlusIcon(gfx::Canvas* canvas, int offset, int size) {
  // Overriden as Chromium incorrectly uses border radius as the width of
  // the button, which *happens* to be correct as the button is round (at the
  // moment). So this passes the correct values in for offset.
  const int fixed_offset = (GetContentsBounds().width() / 2) - (size / 2);
  NewTabButton::PaintPlusIcon(canvas, fixed_offset, size);
}

SkPath BraveNewTabButton::GetNewerMaterialUiButtonPath(float button_y,
                                                       float scale,
                                                       bool extend_to_top,
                                                       bool for_fill) const {
  SkPath path;
  const gfx::Rect contents_bounds = GetContentsBounds();
  path.addRect(0, extend_to_top ? 0 : button_y,
               contents_bounds.width() * scale,
               button_y + contents_bounds.height() * scale);
  path.close();
  return path;
}
