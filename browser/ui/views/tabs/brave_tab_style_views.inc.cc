/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file must be included inside tab_style_views.cc as classes here are
// depending on what's defined in anonymous namespace of tab_style_views.cc

namespace {

////////////////////////////////////////////////////////////////////////////////
// BraveGM2TabStyle
//
class BraveGM2TabStyle : public GM2TabStyleViews {
 public:
  explicit BraveGM2TabStyle(Tab* tab);
  BraveGM2TabStyle(const BraveGM2TabStyle&) = delete;
  BraveGM2TabStyle& operator=(const BraveGM2TabStyle&) = delete;
  ~BraveGM2TabStyle() override = default;

 protected:
  TabStyle::TabColors CalculateColors() const override;

  Tab* tab() { return base::to_address(tab_); }
  const Tab* tab() const { return base::to_address(tab_); }

 private:
  raw_ptr<Tab> tab_;
};

BraveGM2TabStyle::BraveGM2TabStyle(Tab* tab)
    : GM2TabStyleViews(tab), tab_(tab) {}

TabStyle::TabColors BraveGM2TabStyle::CalculateColors() const {
  auto colors = GM2TabStyleViews::CalculateColors();
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
  SkPath GetPath(TabStyle::PathType path_type,
                 float scale,
                 bool force_active = false,
                 TabStyle::RenderUnits render_units =
                     TabStyle::RenderUnits::kPixels) const override;
  TabStyle::SeparatorBounds GetSeparatorBounds(float scale) const override;
  void PaintTab(gfx::Canvas* canvas) const override;

 private:
  bool ShouldShowVerticalTabs() const;
  bool IsInGroupAndNotActive() const;
  SkColor GetTargetTabBackgroundColor(
      TabStyle::TabSelectionState selection_state) const override;
};

BraveVerticalTabStyle::BraveVerticalTabStyle(Tab* tab) : BraveGM2TabStyle(tab) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This class should be used only when the flag is on.";
}

SkPath BraveVerticalTabStyle::GetPath(
    TabStyle::PathType path_type,
    float scale,
    bool force_active,
    TabStyle::RenderUnits render_units) const {
  if (!ShouldShowVerticalTabs()) {
    return BraveGM2TabStyle::GetPath(path_type, scale, force_active,
                                     render_units);
  }

  const int stroke_thickness = GetStrokeThickness();
  gfx::RectF aligned_bounds =
      ScaleAndAlignBounds(tab()->bounds(), scale, stroke_thickness);
  if (tab()->bounds().IsEmpty() || aligned_bounds.IsEmpty()) {
    return {};
  }

#if DCHECK_IS_ON()
  if (tab()->bounds().height() != std::round(aligned_bounds.height() / scale)) {
    DLOG(ERROR) << "We don't want it to be off by 1 dip\n|height|: "
                << tab()->bounds().height() << "\n|aligned bounds height|: "
                << std::round(aligned_bounds.height() / scale);
  }
#endif

  const bool is_pinned = tab()->data().pinned;

  // Calculate the bounds of the actual path.
  float tab_top = aligned_bounds.y();
  float tab_left = aligned_bounds.x();
  float tab_right = aligned_bounds.right();
  float tab_bottom = aligned_bounds.bottom();
  int radius =
      is_pinned ? tabs::kPinnedTabBorderRadius : tabs::kUnpinnedTabBorderRadius;

  if (is_pinned) {
    // Only pinned tabs have border
    if (path_type == TabStyle::PathType::kBorder ||
        path_type == TabStyle::PathType::kFill) {
      // As stroke's coordinate is amid of stroke width, we should set position
      // at 50% of 1 dip.
      tab_top += scale * 0.5;
      tab_left += scale * 0.5;
      tab_right -= scale * 0.5;
      tab_bottom -= scale * 0.5;
    }

    if (path_type == TabStyle::PathType::kInteriorClip) {
      // In order to clip the fill by the stroke thickness, we should set
      // another 1 dip for interior clip.
      tab_top += scale + scale * 0.5;
      tab_left += scale + scale * 0.5;
      tab_right -= scale + scale * 0.5;
      tab_bottom -= scale + scale * 0.5;
      radius -= scale;
    }
  }

  SkPath path;
  path.addRoundRect({tab_left, tab_top, tab_right, tab_bottom}, radius * scale,
                    radius * scale);

  // Convert path to be relative to the tab origin.
  gfx::PointF origin(tab()->origin());
  origin.Scale(scale);
  path.offset(-origin.x(), -origin.y());

  // Possibly convert back to DIPs.
  if (render_units == TabStyle::RenderUnits::kDips && scale != 1.0f) {
    path.transform(SkMatrix::Scale(1.0f / scale, 1.0f / scale));
  }

  return path;
}

TabStyle::SeparatorBounds BraveVerticalTabStyle::GetSeparatorBounds(
    float scale) const {
  if (ShouldShowVerticalTabs()) {
    return {};
  }

  return BraveGM2TabStyle::GetSeparatorBounds(scale);
}

void BraveVerticalTabStyle::PaintTab(gfx::Canvas* canvas) const {
  BraveGM2TabStyle::PaintTab(canvas);
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  if (tab()->data().pinned) {
    const auto* widget = tab()->GetWidget();
    CHECK(widget);
    const SkColor tab_stroke_color =
        widget->GetColorProvider()->GetColor(kColorBraveVerticalTabSeparator);
    PaintBackgroundStroke(canvas, TabActive::kActive, tab_stroke_color);
  }
}

SkColor BraveVerticalTabStyle::GetTargetTabBackgroundColor(
    TabStyle::TabSelectionState selection_state) const {
  if (!ShouldShowVerticalTabs()) {
    return BraveGM2TabStyle::GetTargetTabBackgroundColor(selection_state);
  }

  const ui::ColorProvider* cp = tab()->GetColorProvider();
  if (!cp) {
    return gfx::kPlaceholderColor;
  }

  return cp->GetColor(selection_state == TabStyle::TabSelectionState::kActive
                          ? kColorBraveVerticalTabActiveBackground
                          : kColorBraveVerticalTabInactiveBackground);
}

bool BraveVerticalTabStyle::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowVerticalTabs(tab()->controller()->GetBrowser());
}

}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return std::make_unique<BraveVerticalTabStyle>(tab);
  }

  return std::make_unique<BraveGM2TabStyle>(tab);
}
