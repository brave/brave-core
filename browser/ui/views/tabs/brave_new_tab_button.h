/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_

#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

class TabStrip;
namespace views {
class ButtonListener;
}

class BraveNewTabButton : public NewTabButton {
  METADATA_HEADER(BraveNewTabButton, NewTabButton)

 public:
  // This static members are shared with BraveTabSearchButton
  // TODO(sko) If we could make TabSearchButton inherit BraveNewTabButton,
  // we might not need to do this any more.
  static gfx::Size GetButtonSize();

  BraveNewTabButton(TabStrip* tab_strip, PressedCallback callback);
  ~BraveNewTabButton() override;

 protected:
  TabStrip* tab_strip() { return tab_strip_; }
  const TabStrip* tab_strip() const { return tab_strip_; }

  views::InkDropContainerView* ink_drop_container() {
    return base::to_address(ink_drop_container_);
  }

  // Allow child classes to override PaintFill().
  virtual void OnPaintFill(gfx::Canvas* canvas) const;

  // NewTabButton:
  void PaintIcon(gfx::Canvas* canvas) override;
  void PaintFill(gfx::Canvas* canvas) const override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  SkPath GetBorderPath(const gfx::Point& origin,
                       bool extend_to_top) const override;
  gfx::Insets GetInsets() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
