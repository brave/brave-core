/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file must be included inside tab_style_views.cc as classes here are
// depending on what's defined in anonymous namespace of tab_style_views.cc

namespace {

using tabs::features::HorizontalTabsUpdateEnabled;

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
  TabStyle::TabColors CalculateTargetColors() const override;

 private:
  raw_ptr<Tab> tab_;
};

BraveGM2TabStyle::BraveGM2TabStyle(Tab* tab)
    : GM2TabStyleViews(tab), tab_(tab) {}

TabStyle::TabColors BraveGM2TabStyle::CalculateTargetColors() const {
  auto colors = GM2TabStyleViews::CalculateTargetColors();
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

  gfx::Insets GetContentsInsets() const override;
  TabStyle::SeparatorBounds GetSeparatorBounds(float scale) const override;
  float GetSeparatorOpacity(bool for_layout, bool leading) const override;
  int GetStrokeThickness(bool should_paint_as_active = false) const override;
  void PaintTab(gfx::Canvas* canvas) const override;

 private:
  bool ShouldShowVerticalTabs() const;
  SkColor GetTargetTabBackgroundColor(
      TabStyle::TabSelectionState selection_state,
      bool hovered) const override;
};

BraveVerticalTabStyle::BraveVerticalTabStyle(Tab* tab) : BraveGM2TabStyle(tab) {
}

SkPath BraveVerticalTabStyle::GetPath(
    TabStyle::PathType path_type,
    float scale,
    bool force_active,
    TabStyle::RenderUnits render_units) const {
  if (!HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
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

  if (!ShouldShowVerticalTabs()) {
    // Horizontal tabs should have a visual gap between them, even if their view
    // bounds are touching or slightly overlapping. Create a visual gap by
    // insetting the bounds of the tab by the required gap plus overlap before
    // drawing the rectangle.
    aligned_bounds.Inset(
        gfx::InsetsF::VH(brave_tabs::kHorizontalTabVerticalSpacing * scale,
                         brave_tabs::kHorizontalTabInset * scale));

    // For hit testing, expand the rectangle so that the visual margins around
    // tabs can be used to select the tab. This will ensure that there is no
    // "dead space" between tabs, or between the tab shape and the tab hover
    // card.
    if (path_type == TabStyle::PathType::kHitTest) {
      auto hit_test_outsets =
          gfx::OutsetsF::VH(brave_tabs::kHorizontalTabVerticalSpacing * scale,
                            brave_tabs::kHorizontalTabGap / 2 * scale);

      // We should only extend the hit test bounds into the top margin if the
      // window is maximized or in fullscreen mode. Otherwise, we want the space
      // above the visual tab shape to be available for window-dragging.
      if (!ShouldExtendHitTest()) {
        hit_test_outsets.set_top(0);
      }

      aligned_bounds.Outset(hit_test_outsets);
    }
  }

  const bool is_pinned = tab()->data().pinned;

  // Calculate the bounds of the actual path.
  float tab_top = aligned_bounds.y();
  float tab_left = aligned_bounds.x();
  float tab_right = aligned_bounds.right();
  float tab_bottom = aligned_bounds.bottom();
  int radius = tabs::GetTabCornerRadius(*tab());

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

gfx::Insets BraveVerticalTabStyle::GetContentsInsets() const {
  if (!HorizontalTabsUpdateEnabled()) {
    return BraveGM2TabStyle::GetContentsInsets();
  }

  // Ignore any stroke widths when determining the horizontal contents insets.
  return tab_style()->GetContentsInsets();
}

TabStyle::SeparatorBounds BraveVerticalTabStyle::GetSeparatorBounds(
    float scale) const {
  if (ShouldShowVerticalTabs()) {
    return {};
  }

  if (!HorizontalTabsUpdateEnabled()) {
    return BraveGM2TabStyle::GetSeparatorBounds(scale);
  }

  gfx::SizeF size(tab_style()->GetSeparatorSize());
  size.Scale(scale);
  const gfx::RectF aligned_bounds =
      ScaleAndAlignBounds(tab()->bounds(), scale, GetStrokeThickness());

  // Note: `leading` bounds are used for rect corner radius calculation and so
  // must be non-empty, even if we don't want to show it.
  TabStyle::SeparatorBounds bounds;
  bounds.leading = gfx::RectF(aligned_bounds.right(),
                              (aligned_bounds.height() - size.height()) / 2,
                              size.width(), size.height());
  bounds.trailing = bounds.leading;
  bounds.trailing.set_x(aligned_bounds.right() - size.width());

  gfx::PointF origin(tab()->bounds().origin());
  origin.Scale(scale);
  bounds.trailing.Offset(-origin.x(), -origin.y());

  return bounds;
}

float BraveVerticalTabStyle::GetSeparatorOpacity(bool for_layout,
                                                 bool leading) const {
  if (ShouldShowVerticalTabs() || !HorizontalTabsUpdateEnabled()) {
    return BraveGM2TabStyle::GetSeparatorOpacity(for_layout, leading);
  }

  if (leading) {
    return 0;
  }

  if (tab()->data().pinned) {
    return 0;
  }

  const auto has_visible_background = [](const Tab* const tab) {
    return tab->IsActive() || tab->IsSelected() || tab->IsMouseHovered();
  };

  if (has_visible_background(tab())) {
    return 0;
  }

  const Tab* const next_tab = tab()->controller()->GetAdjacentTab(tab(), 1);

  const float visible_opacity =
      GetHoverInterpolatedSeparatorOpacity(for_layout, next_tab);

  // Show separator if this is the last tab (and is therefore followed by the
  // new tab icon).
  if (!next_tab) {
    return visible_opacity;
  }

  // Don't show separator if there is a group header between this tab and the
  // next.
  if (next_tab->group().has_value() && tab()->group() != next_tab->group()) {
    return 0;
  }

  if (has_visible_background(next_tab)) {
    return 0;
  }

  return visible_opacity;
}

int BraveVerticalTabStyle::GetStrokeThickness(
    bool should_paint_as_active) const {
  if (!HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
    return BraveGM2TabStyle::GetStrokeThickness(should_paint_as_active);
  }
  return 0;
}

void BraveVerticalTabStyle::PaintTab(gfx::Canvas* canvas) const {
  if (ShouldShowVerticalTabs()) {
    // For vertical tabs, bypass the upstream logic to paint theme backgrounds,
    // as this can cause crashes due to the vertical tabstrip living in a
    // different widget hierarchy.
    PaintTabBackground(canvas, GetSelectionState(), IsHoverAnimationActive(),
                       std::nullopt, 0);
  } else {
    BraveGM2TabStyle::PaintTab(canvas);
  }

  if (!HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
    return;
  }

  // Paint a stroke for pinned tabs.
  if (tab()->data().pinned) {
    const auto* widget = tab()->GetWidget();
    CHECK(widget);

    ui::ColorId color_id = widget->ShouldPaintAsActive()
                               ? kColorTabStrokeFrameActive
                               : kColorTabStrokeFrameInactive;
    if (ShouldShowVerticalTabs()) {
      color_id = kColorBraveVerticalTabSeparator;
    }

    SkPath stroke_path =
        GetPath(TabStyle::PathType::kBorder, canvas->image_scale(), false);

    gfx::ScopedCanvas scoped_canvas(canvas);
    float scale = canvas->UndoDeviceScaleFactor();
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(widget->GetColorProvider()->GetColor(color_id));
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setStrokeWidth(scale);
    canvas->DrawPath(stroke_path, flags);
  }
}

SkColor BraveVerticalTabStyle::GetTargetTabBackgroundColor(
    TabStyle::TabSelectionState selection_state,
    bool hovered) const {
  if (!ShouldShowVerticalTabs()) {
    return BraveGM2TabStyle::GetTargetTabBackgroundColor(selection_state,
                                                         hovered);
  }

  if (selection_state == TabStyle::TabSelectionState::kSelected) {
    // Use the same color if th tab is selected via multiselection.
    return BraveGM2TabStyle::GetTargetTabBackgroundColor(selection_state,
                                                         hovered);
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
  return std::make_unique<BraveVerticalTabStyle>(tab);
}
