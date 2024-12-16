/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file must be included inside tab_style_views.cc as classes here are
// depending on what's defined in anonymous namespace of tab_style_views.cc

#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/ui/color/nala/nala_color_id.h"

namespace {

using tabs::features::HorizontalTabsUpdateEnabled;

constexpr auto kPaddingForVerticalTabInTile = 4;

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
// BraveTabStyleViews
//
class BraveTabStyleViews : public TabStyleViewsImpl {
 public:
  explicit BraveTabStyleViews(Tab* tab);
  BraveTabStyleViews(const BraveTabStyleViews&) = delete;
  BraveTabStyleViews& operator=(const BraveTabStyleViews&) = delete;
  ~BraveTabStyleViews() override = default;

 protected:
  TabStyle::TabColors CalculateTargetColors() const override;

 private:
  raw_ptr<Tab> tab_;
};

BraveTabStyleViews::BraveTabStyleViews(Tab* tab)
    : TabStyleViewsImpl(tab), tab_(tab) {}

TabStyle::TabColors BraveTabStyleViews::CalculateTargetColors() const {
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
class BraveVerticalTabStyle : public BraveTabStyleViews {
 public:
  explicit BraveVerticalTabStyle(Tab* tab);
  BraveVerticalTabStyle(const BraveVerticalTabStyle&) = delete;
  BraveVerticalTabStyle& operator=(const BraveVerticalTabStyle&) = delete;
  ~BraveVerticalTabStyle() override = default;

  // BraveTabStyleViews:
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
    : BraveTabStyleViews(tab) {}

SkPath BraveVerticalTabStyle::GetPath(
    TabStyle::PathType path_type,
    float scale,
    bool force_active,
    TabStyle::RenderUnits render_units) const {
  if (!HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
    return BraveTabStyleViews::GetPath(path_type, scale, force_active,
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

    // |aligned_bounds| is tab's bounds(). So, it includes insets also.
    // Shrink height more if it's overlapped.
    if (path_type != TabStyle::PathType::kHitTest) {
      aligned_bounds.Inset(gfx::InsetsF::TLBR(
          0, 0, GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP) * scale, 0));
    }

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

  if (!is_pinned && IsTabTiled(tab()) && path_type != TabStyle::PathType::kHitTest) {
    if (ShouldShowVerticalTabs()) {
      tab()->controller()->IsFirstTabInTile(tab())
          ? tab_top += scale* kPaddingForVerticalTabInTile
          : tab_bottom -= scale * kPaddingForVerticalTabInTile;
      tab_left += scale * kPaddingForVerticalTabInTile;
      tab_right -= scale * kPaddingForVerticalTabInTile;
    } else {
      // Give 2 dip more padding when tab is in tile.
      constexpr auto kPaddingForHorizontalTab = 2;
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
  const bool is_pinned = tab()->data().pinned;
  auto insets = tab_style()->GetContentsInsets();

  if (!is_pinned && ShouldShowVerticalTabs() && IsTabTiled(tab())) {
    const bool is_first_tab = tab()->controller()->IsFirstTabInTile(tab());
    return insets + gfx::Insets::TLBR(
                        is_first_tab ? kPaddingForVerticalTabInTile : 0, 0,
                        is_first_tab ? 0 : kPaddingForVerticalTabInTile, 0);
  }

  if (HorizontalTabsUpdateEnabled()) {
    // Ignore any stroke widths when determining the horizontal contents insets.
    // To make contents vertically align evenly regardless of overlap in non
    // vertical tab, use it as bottom inset in a tab as it's hidden by
    // overlapping.
    return insets +
           gfx::Insets::TLBR(0, 0,
                             ShouldShowVerticalTabs()
                                 ? 0
                                 : GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP),
                             0);
  }

  return BraveTabStyleViews::GetContentsInsets();
}

TabStyle::SeparatorBounds BraveVerticalTabStyle::GetSeparatorBounds(
    float scale) const {
  if (!HorizontalTabsUpdateEnabled()) {
    return BraveTabStyleViews::GetSeparatorBounds(scale);
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
    return BraveTabStyleViews::GetSeparatorOpacity(for_layout, leading);
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
    return BraveTabStyleViews::GetStrokeThickness(should_paint_as_active);
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
    BraveTabStyleViews::PaintTab(canvas);
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
  const ui::ColorProvider* cp = tab()->GetColorProvider();
  if (!cp) {
    return gfx::kPlaceholderColor;
  }

  // Tab in tile doesn't have background in inactive state.
  // In split view tile, we don't have selected tab's background.
  // When any tab in a tile is clicked, the other tab in a same tile
  // is also selected because clicking is start point of dragging.
  // Because of that, whenever click a tab in a tile, the other tab's
  // background is changed as its becomes selected tab.
  // It's not easy to know whether selected state is from clicking or
  // dragging here. As having selected tab state in a tile is not a
  // common state, I think it's fine to not have that state in a tile.
  if (IsTabTiled(tab()) && !tab()->IsActive() && !hovered) {
    return cp->GetColor(ShouldShowVerticalTabs()
                            ? kColorBraveSplitViewTileBackgroundVertical
                            : kColorBraveSplitViewTileBackgroundHorizontal);
  }

  if (!ShouldShowVerticalTabs()) {
    return BraveTabStyleViews::GetTargetTabBackgroundColor(selection_state,
                                                           hovered);
  }

  if (tab()->IsActive()) {
    return cp->GetColor(nala::kColorDesktopbrowserTabbarActiveTabVertical);
  }

  if (hovered) {
    return cp->GetColor(nala::kColorDesktopbrowserTabbarHoverTabVertical);
  }

  if (selection_state == TabStyle::TabSelectionState::kSelected) {
    // Use the same color if th tab is selected via multiselection.
    return BraveTabStyleViews::GetTargetTabBackgroundColor(selection_state,
                                                           hovered);
  }

  return cp->GetColor(kColorBraveVerticalTabInactiveBackground);
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
