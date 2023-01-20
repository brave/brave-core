/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_

#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

class TabStrip;
namespace views {
class ButtonListener;
class Label;
}

class BraveNewTabButton : public NewTabButton {
 public:
  // These static members are shared with BraveTabSearchButton
  // TODO(sko) If we could make TabSearchButton inherit BraveNewTabButton,
  // we might not need these any more.
  static const gfx::Size kButtonSize;
  static constexpr int kHeightForVerticalTabs = 50;
  static SkPath GetBorderPath(const gfx::Point& origin,
                              float scale,
                              bool extend_to_top,
                              int border_radius,
                              const gfx::Size& contents_bounds);
  BraveNewTabButton(TabStrip* tab_strip, PressedCallback callback);
  BraveNewTabButton(const BraveNewTabButton&) = delete;
  BraveNewTabButton& operator=(const BraveNewTabButton&) = delete;
  ~BraveNewTabButton() override;

  void SetShortcutText(const std::u16string& text);

  // NewTabButton:
  void FrameColorsChanged() override;
  void Layout() override;

 protected:
  // NewTabButton:
  void PaintIcon(gfx::Canvas* canvas) override;
  void PaintFill(gfx::Canvas* canvas) const override;

 private:
  gfx::Size CalculatePreferredSize() const override;
  SkPath GetBorderPath(const gfx::Point& origin,
                       float scale,
                       bool extend_to_top) const override;
  gfx::Insets GetInsets() const override;

  raw_ptr<views::Label> text_ = nullptr;
  raw_ptr<views::Label> shortcut_text_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
