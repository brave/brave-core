/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "brave/browser/ui/color/brave_color_id.h"
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

////////////////////////////////////////////////////////////////////////////////
// BraveGM2TabStyle
//
class BraveGM2TabStyle : public GM2TabStyle {
 public:
  explicit BraveGM2TabStyle(Tab* tab);
  BraveGM2TabStyle(const BraveGM2TabStyle&) = delete;
  BraveGM2TabStyle& operator=(const BraveGM2TabStyle&) = delete;
  ~BraveGM2TabStyle() override = default;

 protected:
  TabStyle::TabColors CalculateColors() const override;
  const gfx::FontList& GetFontList() const override;

  Tab* tab() { return base::to_address(tab_); }
  const Tab* tab() const { return base::to_address(tab_); }

 private:
  raw_ptr<Tab> tab_;
  gfx::FontList active_tab_font_;
};

BraveGM2TabStyle::BraveGM2TabStyle(Tab* tab)
    : GM2TabStyle(tab),
      tab_(tab),
      active_tab_font_(
          normal_font_.DeriveWithWeight(gfx::Font::Weight::MEDIUM)) {}

TabStyle::TabColors BraveGM2TabStyle::CalculateColors() const {
  auto colors = GM2TabStyle::CalculateColors();
  const SkColor inactive_non_hovered_fg_color = SkColorSetA(
      colors.foreground_color,
      gfx::Tween::IntValueBetween(0.7, SK_AlphaTRANSPARENT, SK_AlphaOPAQUE));
  SkColor final_fg_color = (tab_->IsActive() || tab_->mouse_hovered())
                               ? colors.foreground_color
                               : inactive_non_hovered_fg_color;
  SkColor final_bg_color = colors.background_color;
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

////////////////////////////////////////////////////////////////////////////////
// BraveVerticalTabStyle
//
// This class deals with tab styling when vertical tab strip feature flag is
// enabled.
//
class BraveVerticalTabStyle : public BraveGM2TabStyle {
 public:
  explicit BraveVerticalTabStyle(Tab* tab);
  BraveVerticalTabStyle(const BraveVerticalTabStyle&) = delete;
  BraveVerticalTabStyle& operator=(const BraveVerticalTabStyle&) = delete;
  ~BraveVerticalTabStyle() override = default;

  // BraveGM2TabStyle:
  SkPath GetPath(
      PathType path_type,
      float scale,
      bool force_active = false,
      RenderUnits render_units = RenderUnits::kPixels) const override;
  SeparatorBounds GetSeparatorBounds(float scale) const override;
  void PaintTab(gfx::Canvas* canvas) const override;

 private:
  bool ShouldShowVerticalTabs() const;
  bool IsInGroupAndNotActive() const;
};

BraveVerticalTabStyle::BraveVerticalTabStyle(Tab* tab) : BraveGM2TabStyle(tab) {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This class should be used only when the flag is on.";
}

SkPath BraveVerticalTabStyle::GetPath(PathType path_type,
                                      float scale,
                                      bool force_active,
                                      RenderUnits render_units) const {
  if (!ShouldShowVerticalTabs())
    return BraveGM2TabStyle::GetPath(path_type, scale, force_active,
                                     render_units);

  const int stroke_thickness = GetStrokeThickness();
  gfx::RectF aligned_bounds =
      ScaleAndAlignBounds(tab()->bounds(), scale, stroke_thickness);

  constexpr int kHorizontalInset = BraveTabGroupHeader::kPaddingForGroup;

  // Calculate the bounds of the actual path.
  float tab_top = aligned_bounds.y();
  float tab_left = aligned_bounds.x() + kHorizontalInset * scale;
  float tab_right = aligned_bounds.right() - kHorizontalInset * scale;
  float tab_bottom = aligned_bounds.bottom();

  const float stroke_adjustment = stroke_thickness * scale;
  if (path_type == PathType::kInteriorClip) {
    // Inside of the border runs |stroke_thickness| inside the outer edge.
    tab_left += stroke_adjustment;
    tab_right -= stroke_adjustment;
    tab_top += stroke_adjustment;
    tab_bottom -= stroke_adjustment;
  } else if (path_type == PathType::kFill || path_type == PathType::kBorder) {
    tab_left += 0.5f * stroke_adjustment;
    tab_right -= 0.5f * stroke_adjustment;
    tab_top += 0.5f * stroke_adjustment;
    tab_bottom -= 0.5f * stroke_adjustment;
  }

  SkPath path;
  path.addRoundRect({tab_left, tab_top, tab_right, tab_bottom},
                    kHorizontalInset * scale, kHorizontalInset * scale);

  // Convert path to be relative to the tab origin.
  gfx::PointF origin(tab()->origin());
  origin.Scale(scale);
  path.offset(-origin.x(), -origin.y());

  // Possibly convert back to DIPs.
  if (render_units == RenderUnits::kDips && scale != 1.0f)
    path.transform(SkMatrix::Scale(1.0f / scale, 1.0f / scale));

  return path;
}

TabStyle::SeparatorBounds BraveVerticalTabStyle::GetSeparatorBounds(
    float scale) const {
  if (ShouldShowVerticalTabs())
    return {};

  return BraveGM2TabStyle::GetSeparatorBounds(scale);
}

void BraveVerticalTabStyle::PaintTab(gfx::Canvas* canvas) const {
  BraveGM2TabStyle::PaintTab(canvas);
  if (ShouldShowVerticalTabs() && (tab()->IsActive() || IsHoverActive())) {
    const auto* widget = tab()->GetWidget();
    DCHECK(widget);
    const SkColor tab_stroke_color =
        widget->GetColorProvider()->GetColor(kColorBraveVerticalTabSeparator);
    PaintBackgroundStroke(canvas, TabActive::kActive, tab_stroke_color);
    return;
  }
}

bool BraveVerticalTabStyle::ShouldShowVerticalTabs() const {
  return tabs::features::ShouldShowVerticalTabs(
      tab()->controller()->GetBrowser());
}

}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return std::make_unique<BraveVerticalTabStyle>(tab);

  return std::make_unique<BraveGM2TabStyle>(tab);
}
