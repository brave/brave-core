/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file must be included inside tab_style_views.cc as classes here are
// depending on what's defined in anonymous namespace of tab_style_views.cc

#include "brave/browser/ui/tabs/split_view_browser_data.h"

namespace {

using tabs::features::HorizontalTabsUpdateEnabled;

// Returns a value indicating if the browser frame view is "condensed", i.e.
// that its frame border is somehow collapsed, as in fullscreen or when
// maximized, or in Linux when caption buttons and the title bar are not
// displayed. For tabs, this is important for Fitts' law; ensuring that when the
// browser occupies the full screen, tabs can be selected by moving the pointer
// to the edge of the screen.
bool IsBrowserFrameCondensed(const Browser* browser) {
  if (!browser) {
    return false;
  }
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  DCHECK(browser_view);
  return browser_view->frame()->GetFrameView()->IsFrameCondensed();
}

////////////////////////////////////////////////////////////////////////////////
// BraveGM2TabStyle
//
class BraveGM2TabStyle : public TabStyleViewsImpl {
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
    : TabStyleViewsImpl(tab), tab_(tab) {}

TabStyle::TabColors BraveGM2TabStyle::CalculateTargetColors() const {
  auto colors = TabStyleViewsImpl::CalculateTargetColors();
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
  bool IsTabTiled(const Tab* tab) const;

  SkColor GetTargetTabBackgroundColor(
      TabStyle::TabSelectionState selection_state,
      bool hovered) const override;
};

BraveVerticalTabStyle::BraveVerticalTabStyle(Tab* tab)
    : BraveGM2TabStyle(tab) {}

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

      // Note that upstream's `ShouldExtendHitTest` does not currently take into
      // account some "condensed" frame scenarios on Linux.
      bool frame_condensed =
          IsBrowserFrameCondensed(tab()->controller()->GetBrowser());

      // We should only extend the hit test bounds into the top margin if the
      // browser frame is "condensed" (e.g. maximized, fullscreen, or otherwise
      // occupying the entire screen area). Otherwise, we want the space above
      // the visual tab shape to be available for window-dragging.
      if (!frame_condensed) {
        hit_test_outsets.set_top(0);
      }

      // We also want the first tab (taking RTL into account) to be selectable
      // in maximized or fullscreen mode by clicking at the very edge of the
      // screen.
      if (frame_condensed && tab()->controller()->IsTabFirst(tab())) {
        if (tab()->GetMirrored()) {
          hit_test_outsets.set_right(brave_tabs::kHorizontalTabInset * scale);
        } else {
          hit_test_outsets.set_left(brave_tabs::kHorizontalTabInset * scale);
        }
      }

      aligned_bounds.Outset(hit_test_outsets);
    }
  }

  const bool is_tab_tiled = IsTabTiled(tab());

  if (is_tab_tiled && !tab()->IsActive() &&
      (path_type == TabStyle::PathType::kBorder ||
       path_type == TabStyle::PathType::kFill)) {
    // We don't want inactive tab in a tile to have border or fill.
    return SkPath();
  }

  const bool is_pinned = tab()->data().pinned;

  // Calculate the bounds of the actual path.
  float tab_top = aligned_bounds.y();
  float tab_left = aligned_bounds.x();
  float tab_right = aligned_bounds.right();
  float tab_bottom = aligned_bounds.bottom();
  int radius = tabs::GetTabCornerRadius(*tab());

  // For hit testing, the tab shape should not be rounded, as that would leave
  // small hit test gaps between adjacent tabs at their corners.
  if (path_type == TabStyle::PathType::kHitTest) {
    radius = 0;
  }

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

  if (is_tab_tiled && path_type != TabStyle::PathType::kHitTest) {
    if (ShouldShowVerticalTabs()) {
      constexpr auto kPaddingForVerticalTab = 4;
      tab_top += scale * kPaddingForVerticalTab;
      tab_bottom -= scale * kPaddingForVerticalTab;
      tab_left += scale * kPaddingForVerticalTab;
      tab_right -= scale * kPaddingForVerticalTab;
    } else {
      // As the horizontal tab has padding already we only gives 1 dip padding.
      // Accumulative padding will be 4 dips.
      constexpr auto kPaddingForHorizontalTab = 1;
      tab_top += scale * kPaddingForHorizontalTab;
      tab_bottom -= scale * kPaddingForHorizontalTab;
      tab()->controller()->IsFirstTabInTile(tab())
          ? tab_left += scale* kPaddingForHorizontalTab
          : tab_right -= scale * kPaddingForHorizontalTab;
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
  if (ShouldShowVerticalTabs()) {
    return 0;
  }

  if (IsTabTiled(tab())) {
    return 0;
  }

  const Tab* const next_tab = tab()->controller()->GetAdjacentTab(tab(), 1);
  const auto is_next_tab_tiled = IsTabTiled(next_tab);
  if (is_next_tab_tiled) {
    return 0;
  }

  if (!HorizontalTabsUpdateEnabled()) {
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

bool BraveVerticalTabStyle::IsTabTiled(const Tab* tab) const {
  if (!tab) {
    return false;
  }

  auto is_tab_tiled = false;
  if (auto* browser = tab->controller()->GetBrowser()) {
    // browser can be null during tests.
    if (auto* data = SplitViewBrowserData::FromBrowser(browser);
        data && tab->controller()->IsTabTiled(tab)) {
      is_tab_tiled = true;
    }
  }
  return is_tab_tiled;
}

}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  return std::make_unique<BraveVerticalTabStyle>(tab);
}
