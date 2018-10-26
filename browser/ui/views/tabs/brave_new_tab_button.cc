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
