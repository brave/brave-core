/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_style_views.h"

#define BRAVE_GM2_TAB_STYLE_H protected:
#define CreateForTab CreateForTab_ChromiumImpl
#include "../../../../../../../chrome/browser/ui/views/tabs/tab_style_views.cc"
#undef CreateForTab
#undef BRAVE_GM2_TAB_STYLE_H

namespace {

class BraveGM2TabStyle : public GM2TabStyle {
 public:
  explicit BraveGM2TabStyle(Tab* tab);
  BraveGM2TabStyle(const BraveGM2TabStyle&) = delete;
  BraveGM2TabStyle& operator=(const BraveGM2TabStyle&) = delete;

 protected:
  TabStyle::TabColors CalculateColors() const override;
  const gfx::FontList& GetFontList() const override;

 private:
  gfx::FontList active_tab_font_;
};

BraveGM2TabStyle::BraveGM2TabStyle(Tab* tab)
    : GM2TabStyle(tab),
      active_tab_font_(
          normal_font_.DeriveWithWeight(gfx::Font::Weight::MEDIUM)) {}

TabStyle::TabColors BraveGM2TabStyle::CalculateColors() const {
  auto colors = GM2TabStyle::CalculateColors();
  const SkColor inactive_non_hovered_fg_color = SkColorSetA(
      colors.foreground_color,
      gfx::Tween::IntValueBetween(0.7, SK_AlphaTRANSPARENT, SK_AlphaOPAQUE));
  const SkColor final_fg_color = (tab_->IsActive() || tab_->mouse_hovered())
                                     ? colors.foreground_color
                                     : inactive_non_hovered_fg_color;
  return {final_fg_color, colors.background_color};
}

const gfx::FontList& BraveGM2TabStyle::GetFontList() const {
  const auto& font_list = GM2TabStyle::GetFontList();
  if (&font_list == &normal_font_ && tab_->IsActive()) {
    return active_tab_font_;
  }
  return font_list;
}

}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  return std::make_unique<BraveGM2TabStyle>(tab);
}
