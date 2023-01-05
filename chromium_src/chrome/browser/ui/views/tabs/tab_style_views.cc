/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

#define BRAVE_GM2_TAB_STYLE_H \
 protected:                   \
  virtual
#define CreateForTab CreateForTab_ChromiumImpl
#include "src/chrome/browser/ui/views/tabs/tab_style_views.cc"
#undef CreateForTab
#undef BRAVE_GM2_TAB_STYLE_H

namespace {

class BraveGM2TabStyle : public GM2TabStyle {
 public:
  explicit BraveGM2TabStyle(Tab* tab);
  BraveGM2TabStyle(const BraveGM2TabStyle&) = delete;
  BraveGM2TabStyle& operator=(const BraveGM2TabStyle&) = delete;

 protected:
  SkPath GetPath(
      PathType path_type,
      float scale,
      bool force_active = false,
      RenderUnits render_units = RenderUnits::kPixels) const override;
  TabStyle::TabColors CalculateColors() const override;
  const gfx::FontList& GetFontList() const override;
  SeparatorBounds GetSeparatorBounds(float scale) const override;
  void PaintTab(gfx::Canvas* canvas) const override;

 private:
  bool IsVerticalTab() const {
    if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      return false;

    return tabs::features::ShouldShowVerticalTabs(
        tab_->controller()->GetBrowser());
  }

  bool IsInactiveAndInGroup() const {
    return !tab_->IsActive() && tab_->group().has_value();
  }

  raw_ptr<Tab> tab_;
  gfx::FontList active_tab_font_;
};

BraveGM2TabStyle::BraveGM2TabStyle(Tab* tab)
    : GM2TabStyle(tab),
      tab_(tab),
      active_tab_font_(
          normal_font_.DeriveWithWeight(gfx::Font::Weight::MEDIUM)) {}

SkPath BraveGM2TabStyle::GetPath(PathType path_type,
                                 float scale,
                                 bool force_active,
                                 RenderUnits render_units) const {
  if (!IsVerticalTab())
    return GM2TabStyle::GetPath(path_type, scale, force_active, render_units);

  // Don't paint border.
  if (path_type == PathType::kBorder)
    return {};

  gfx::RectF aligned_bounds = ScaleAndAlignBounds(tab_->bounds(), scale, 0);

  constexpr int kHorizontalInset = BraveTabGroupHeader::kPaddingForGroup;

  // Calculate the bounds of the actual path.
  float tab_top = aligned_bounds.y();
  float tab_left = aligned_bounds.x() + kHorizontalInset * scale;
  float tab_right = aligned_bounds.right() - kHorizontalInset * scale;
  float tab_bottom = aligned_bounds.bottom();

  SkPath path;
  path.addRoundRect({tab_left, tab_top, tab_right, tab_bottom},
                    kHorizontalInset * scale, kHorizontalInset * scale);

  // Convert path to be relative to the tab origin.
  gfx::PointF origin(tab_->origin());
  origin.Scale(scale);
  path.offset(-origin.x(), -origin.y());

  // Possibly convert back to DIPs.
  if (render_units == RenderUnits::kDips && scale != 1.0f)
    path.transform(SkMatrix::Scale(1.0f / scale, 1.0f / scale));

  return path;
}

TabStyle::TabColors BraveGM2TabStyle::CalculateColors() const {
  auto colors = GM2TabStyle::CalculateColors();
  const SkColor inactive_non_hovered_fg_color = SkColorSetA(
      colors.foreground_color,
      gfx::Tween::IntValueBetween(0.7, SK_AlphaTRANSPARENT, SK_AlphaOPAQUE));
  SkColor final_fg_color = (tab_->IsActive() || tab_->mouse_hovered())
                               ? colors.foreground_color
                               : inactive_non_hovered_fg_color;
  SkColor final_bg_color = colors.background_color;
  if (IsVerticalTab() && IsInactiveAndInGroup()) {
    final_fg_color = BraveTabGroupHeader::GetDarkerColorForGroup(
        tab_->group().value(), tab_->controller(),
        tab_->GetNativeTheme()->ShouldUseDarkColors());
    final_bg_color = SK_ColorTRANSPARENT;
  }

  return {final_fg_color, final_bg_color, colors.focus_ring_color,
          colors.close_button_focus_ring_color};
}

const gfx::FontList& BraveGM2TabStyle::GetFontList() const {
  const auto& font_list = GM2TabStyle::GetFontList();
  if (&font_list == &normal_font_ && tab_->IsActive()) {
    return active_tab_font_;
  }
  return font_list;
}

BraveGM2TabStyle::SeparatorBounds BraveGM2TabStyle::GetSeparatorBounds(
    float scale) const {
  if (IsVerticalTab())
    return {};

  return GM2TabStyle::GetSeparatorBounds(scale);
}

void BraveGM2TabStyle::PaintTab(gfx::Canvas* canvas) const {
  if (!IsVerticalTab() || !IsInactiveAndInGroup()) {
    GM2TabStyle::PaintTab(canvas);
    return;
  }

  // When a tab is in a group while vertical tab is enabled, make tab's
  // background transparent so that the group's background can be visible
  // instead.
  // Skip painting background for inactive tab and paint throbbing background.
  const float throb_value = GetThrobValue();
  if (throb_value <= 0)
    return;

  absl::optional<int> active_tab_fill_id;
  int active_tab_y_inset = 0;
  if (tab_->GetThemeProvider()->HasCustomImage(IDR_THEME_TOOLBAR)) {
    active_tab_fill_id = IDR_THEME_TOOLBAR;
    active_tab_y_inset = GetStrokeThickness(true);
  }
  canvas->SaveLayerAlpha(base::ClampRound<uint8_t>(throb_value * 0xff),
                         tab_->GetLocalBounds());
  PaintTabBackground(canvas, TabActive::kActive, active_tab_fill_id,
                     active_tab_y_inset);
  canvas->Restore();
}

}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  return std::make_unique<BraveGM2TabStyle>(tab);
}
