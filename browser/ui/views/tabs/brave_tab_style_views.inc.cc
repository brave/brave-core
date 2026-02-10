/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file must be included inside tab_style_views.cc as classes here are
// depending on what's defined in anonymous namespace of tab_style_views.cc

#include "base/check.h"
#include "base/dcheck_is_on.h"
#include "base/logging.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace {

using tabs::HorizontalTabsUpdateEnabled;

constexpr auto kPaddingForVerticalTabInTile = 4;
constexpr int kPaddingForHorizontalTabInTile = 4;

// Returns a value indicating if the browser frame view is "condensed", i.e.
// that its frame border is somehow collapsed, as in fullscreen or when
// maximized, or in Linux when caption buttons and the title bar are not
// displayed. For tabs, this is important for Fitts' law; ensuring that when the
// browser occupies the full screen, tabs can be selected by moving the pointer
// to the edge of the screen.
bool IsBrowserFrameCondensed(const BrowserWindowInterface* browser) {
  if (!browser) {
    return false;
  }
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  DCHECK(browser_view);
  return browser_view->browser_widget()->GetFrameView()->IsFrameCondensed();
}

////////////////////////////////////////////////////////////////////////////////
// BraveVerticalTabStyle
//
// This class deals with tab styling when vertical tab strip feature flag is
// enabled.
//
class BraveVerticalTabStyle : public TabStyleViewsImpl {
 public:
  explicit BraveVerticalTabStyle(Tab* tab);
  BraveVerticalTabStyle(const BraveVerticalTabStyle&) = delete;
  BraveVerticalTabStyle& operator=(const BraveVerticalTabStyle&) = delete;
  ~BraveVerticalTabStyle() override = default;

  // A method that returns a path considering |inset_tab_accent_area|.
  // This is used to inset the bounds for Tab Accent icon area.
  SkPath GetPath(TabStyle::PathType path_type,
                 float scale,
                 const TabPathFlags& flags,
                 bool inset_tab_accent_area) const;

  // TabStyleViewsImpl:
  SkPath GetPath(TabStyle::PathType path_type,
                 float scale,
                 const TabPathFlags& flags) const override;
  bool IsHovering() const override;
  gfx::Insets GetContentsInsets() const override;
  TabStyle::SeparatorBounds GetSeparatorBounds(float scale) const override;
  float GetSeparatorOpacity(bool for_layout, bool leading) const override;
  int GetStrokeThickness(bool should_paint_as_active = false) const override;
  void PaintTab(gfx::Canvas* canvas) const override;
  TabStyle::TabColors CalculateTargetColors() const override;
  SkColor GetCurrentTabBackgroundColor(
      TabStyle::TabSelectionState selection_state,
      bool hovered) const override;

 private:
  bool ShouldShowVerticalTabs() const;

  // true when |tab| is shown in the split view(with SideBySide or brave split
  // view)
  bool IsSplitTab(const Tab* tab) const;

  // true when |tab| is shown at the beginning of split view.
  bool IsStartSplitTab(const Tab* tab) const;

  // Return tab background color for certain tab states.
  std::optional<SkColor> GetTargetTabBackgroundColor(
      TabStyle::TabSelectionState selection_state,
      bool hovered) const;

  // Paints the container accent (border, left stripe, and icon) for tabs in
  // special mode, such as Containers.
  void PaintTabAccentBackground(gfx::Canvas* canvas) const;
  void PaintTabAccentBorder(gfx::Canvas* canvas) const;
  void PaintTabAccentIcon(gfx::Canvas* canvas) const;

  // This is called for the |bounds| returned by
  // ScaleAndAlignBounds() to inset the bounds for Tab Accent icon area.
  gfx::RectF InsetAlignedBoundsForTabAccent(const gfx::RectF& bounds,
                                            float scale) const;
};

BraveVerticalTabStyle::BraveVerticalTabStyle(Tab* tab)
    : TabStyleViewsImpl(tab) {}

bool BraveVerticalTabStyle::IsHovering() const {
  // Upstream gives true when the tab is in split tab and another tab is
  // hovered. But, we only want to show hover effect for currently hovered tab.
  return tab()->mouse_hovered();
}

SkPath BraveVerticalTabStyle::GetPath(TabStyle::PathType path_type,
                                      float scale,
                                      const TabPathFlags& flags) const {
  return GetPath(path_type, scale, flags, /*inset_tab_accent_area=*/true);
}

SkPath BraveVerticalTabStyle::GetPath(TabStyle::PathType path_type,
                                      float scale,
                                      const TabPathFlags& flags,
                                      bool inset_tab_accent_area) const {
  if (!HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
    return TabStyleViewsImpl::GetPath(path_type, scale, flags);
  }

  const int stroke_thickness = GetStrokeThickness();
  gfx::RectF aligned_bounds =
      ScaleAndAlignBounds(tab()->bounds(), scale, stroke_thickness);
  if (tab()->bounds().IsEmpty() || aligned_bounds.IsEmpty()) {
    return {};
  }

  if (inset_tab_accent_area) {
    aligned_bounds = InsetAlignedBoundsForTabAccent(aligned_bounds, scale);
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
        gfx::InsetsF::VH(tabs::kHorizontalTabVerticalSpacing * scale,
                         tabs::kHorizontalTabInset * scale));

    // |aligned_bounds| is tab's bounds(). So, it includes insets also.
    // Shrink height more if it's overlapped.
    if (path_type != TabStyle::PathType::kHitTest) {
      aligned_bounds.Inset(gfx::InsetsF::TLBR(
          0, 0,
          GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap) * scale,
          0));
    }

    // For hit testing, expand the rectangle so that the visual margins around
    // tabs can be used to select the tab. This will ensure that there is no
    // "dead space" between tabs, or between the tab shape and the tab hover
    // card.
    if (path_type == TabStyle::PathType::kHitTest) {
      auto hit_test_outsets =
          gfx::OutsetsF::VH(tabs::kHorizontalTabVerticalSpacing * scale,
                            tabs::kHorizontalTabGap / 2 * scale);

      // Note that upstream's `ShouldExtendHitTest` does not currently take into
      // account some "condensed" frame scenarios on Linux.
      bool frame_condensed = IsBrowserFrameCondensed(
          tab()->controller()->GetBrowserWindowInterface());

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
          hit_test_outsets.set_right(tabs::kHorizontalTabInset * scale);
        } else {
          hit_test_outsets.set_left(tabs::kHorizontalTabInset * scale);
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

  if (!is_pinned && IsSplitTab(tab()) &&
      path_type != TabStyle::PathType::kHitTest) {
    if (ShouldShowVerticalTabs()) {
      IsStartSplitTab(tab())
          ? tab_top += scale* kPaddingForVerticalTabInTile
          : tab_bottom -= scale * kPaddingForVerticalTabInTile;
      tab_left += scale * kPaddingForVerticalTabInTile;
      tab_right -= scale * kPaddingForVerticalTabInTile;
    } else {
      constexpr int kAdditionalVerticalPadding =
          tabs::kHorizontalSplitViewTabVerticalSpacing -
          tabs::kHorizontalTabGap;
      tab_top += scale * kAdditionalVerticalPadding;
      tab_bottom -= scale * kAdditionalVerticalPadding;

      IsStartSplitTab(tab())
          ? tab_left += scale* kPaddingForHorizontalTabInTile
          : tab_right -= scale * kPaddingForHorizontalTabInTile;
    }
  }

  SkPathBuilder builder(
      SkPath::RRect({tab_left, tab_top, tab_right, tab_bottom}, radius * scale,
                    radius * scale));

  // Convert path to be relative to the tab origin.
  gfx::PointF origin(tab()->origin());
  origin.Scale(scale);
  builder.offset(-origin.x(), -origin.y());

  // Possibly convert back to DIPs.
  if (flags.render_units == TabStyle::RenderUnits::kDips && scale != 1.0f) {
    builder.transform(SkMatrix::Scale(1.0f / scale, 1.0f / scale));
  }

  return builder.detach();
}

gfx::Insets BraveVerticalTabStyle::GetContentsInsets() const {
  const bool is_pinned = tab()->data().pinned;
  const bool is_vertical_tabs = ShouldShowVerticalTabs();

  auto add_extra_left_padding = [](gfx::Insets& insets) {
    // As close button has more padding, it seems favicon is too close to
    // the left edge of the tab left border comppared with close button.
    // Give additional left padding to make both visible with same space
    // from tab border. See
    // https://www.github.com/brave/brave-browser/issues/30469.
    insets.set_left(insets.left() + BraveTab::kExtraLeftPadding);
  };

  auto add_left_padding_for_accent_icon = [&](gfx::Insets& insets) {
    auto* brave_tab = static_cast<const BraveTab*>(tab());
    if (brave_tab->ShouldPaintTabAccent() &&
        brave_tab->ShouldShowLargeAccentIcon()) {
      insets.set_left(BraveTab::kTabAccentIconAreaWidth + insets.left());
    }
  };

  auto insets = tab_style()->GetContentsInsets();
  add_left_padding_for_accent_icon(insets);
  add_extra_left_padding(insets);

  if (!is_pinned && is_vertical_tabs && IsSplitTab(tab())) {
    const bool is_first_tab = IsStartSplitTab(tab());
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
           gfx::Insets::TLBR(
               0, 0,
               is_vertical_tabs
                   ? 0
                   : GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap),
               0);
  }

  auto result = TabStyleViewsImpl::GetContentsInsets();
  add_left_padding_for_accent_icon(result);
  add_extra_left_padding(result);
  return result;
}

TabStyle::SeparatorBounds BraveVerticalTabStyle::GetSeparatorBounds(
    float scale) const {
  if (!HorizontalTabsUpdateEnabled()) {
    return TabStyleViewsImpl::GetSeparatorBounds(scale);
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

  if (IsSplitTab(tab())) {
    return 0;
  }

  const Tab* const next_tab = tab()->controller()->GetAdjacentTab(tab(), 1);
  const auto is_next_tab_tiled = IsSplitTab(next_tab);
  if (is_next_tab_tiled) {
    return 0;
  }

  if (!HorizontalTabsUpdateEnabled()) {
    return TabStyleViewsImpl::GetSeparatorOpacity(for_layout, leading);
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
    return TabStyleViewsImpl::GetStrokeThickness(should_paint_as_active);
  }
  return 0;
}

void BraveVerticalTabStyle::PaintTab(gfx::Canvas* canvas) const {
  const auto* brave_tab = static_cast<const BraveTab*>(tab());
  CHECK(brave_tab);
  if (ShouldShowVerticalTabs()) {
    // For vertical tabs, bypass the upstream logic to paint theme backgrounds,
    // as this can cause crashes due to the vertical tabstrip living in a
    // different widget hierarchy.
    PaintTabBackground(canvas, GetSelectionState(), IsHoverAnimationActive(),
                       std::nullopt, 0);
  } else {
    TabStyleViewsImpl::PaintTab(canvas);
  }

  const bool should_paint_tab_accent = brave_tab->ShouldPaintTabAccent();
  if (!HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs() &&
      !should_paint_tab_accent) {
    // No additional painting is needed.
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

    gfx::ScopedCanvas scoped_canvas(canvas);
    float scale = canvas->UndoDeviceScaleFactor();

    SkPath stroke_path =
        GetPath(TabStyle::PathType::kBorder, scale, /*flags=*/{});

    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(widget->GetColorProvider()->GetColor(color_id));
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setStrokeWidth(scale);
    canvas->DrawPath(stroke_path, flags);
  }

  // Paint tab accent if needed.
  if (should_paint_tab_accent) {
    PaintTabAccentBackground(canvas);
    PaintTabAccentBorder(canvas);
    PaintTabAccentIcon(canvas);
  }
}

void BraveVerticalTabStyle::PaintTabAccentBackground(
    gfx::Canvas* canvas) const {
  const auto* brave_tab = static_cast<const BraveTab*>(tab());
  CHECK(brave_tab);

  auto accent_color = brave_tab->GetTabAccentColor();
  if (!accent_color.has_value()) {
    return;
  }

  gfx::ScopedCanvas scoped_canvas(canvas);
  float scale = canvas->UndoDeviceScaleFactor();

  // Clip to the full tab shape so we never paint outside the tab.
  SkPath clip_path = GetPath(TabStyle::PathType::kFill, scale,
                             /*flags=*/{}, /*inset_tab_accent_area=*/false);
  canvas->ClipPath(clip_path, /*do_anti_alias=*/true);

  // Paint accent only in the region outside the inset path (e.g. accent icon
  // area). GetPath(..., true) returns the tab shape with left inset.
  SkPath inset_path = GetPath(TabStyle::PathType::kFill, scale,
                              /*flags=*/{}, /*inset_tab_accent_area=*/true);
  SkPath inverse_path = inset_path.makeToggleInverseFillType();

  cc::PaintFlags fill_flags;
  fill_flags.setAntiAlias(true);
  fill_flags.setColor(accent_color.value());
  fill_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawPath(inverse_path, fill_flags);
}

void BraveVerticalTabStyle::PaintTabAccentBorder(gfx::Canvas* canvas) const {
  const auto* brave_tab = static_cast<const BraveTab*>(tab());
  CHECK(brave_tab);

  auto accent_color = brave_tab->GetTabAccentColor();
  if (!accent_color.has_value()) {
    return;
  }

  gfx::ScopedCanvas scoped_canvas(canvas);
  float scale = canvas->UndoDeviceScaleFactor();

  SkPath border_path = GetPath(TabStyle::PathType::kBorder, scale,
                               /*flags=*/{}, /*inset_tab_accent_area=*/false);

  cc::PaintFlags border_flags;
  border_flags.setAntiAlias(true);
  border_flags.setColor(accent_color.value());
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawPath(border_path, border_flags);
}

void BraveVerticalTabStyle::PaintTabAccentIcon(gfx::Canvas* canvas) const {
  auto* brave_tab = static_cast<const BraveTab*>(tab());
  auto accent_icon = brave_tab->GetTabAccentIcon();

  // getBounds() call to find the actual border bounds.
  auto bounds = GetPath(TabStyle::PathType::kBorder, 1, /*flags=*/{},
                        /*inset_tab_accent_area=*/false)
                    .getBounds();

  if (!brave_tab->ShouldShowLargeAccentIcon()) {
    // Small icon is painted on the left-bottom corner of the tab.
    // We need 16x16 bounding circle and centered icon in it.
    constexpr auto circle_size = 16;
    int center_x = bounds.x() + circle_size / 2;
    int center_y = bounds.bottom() - circle_size / 2;
    if (auto background_color = brave_tab->GetTabAccentColor();
        background_color.has_value()) {
      cc::PaintFlags flags;
      flags.setAntiAlias(true);
      flags.setColor(background_color.value());
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->DrawCircle(gfx::PointF(center_x, center_y), 8, flags);
    }

    if (accent_icon.IsEmpty()) {
      return;
    }

    constexpr int dest_image_size = 12;
    auto image =
        brave_tab->GetTabAccentIcon().Rasterize(tab()->GetColorProvider());
    canvas->DrawImageInt(image, 0, 0, image.width(), image.height(),
                         center_x - dest_image_size / 2,
                         center_y - dest_image_size / 2, dest_image_size,
                         dest_image_size, true);
    return;
  }

  if (accent_icon.IsEmpty()) {
    return;
  }

  // Large accent icon is painted in center of the the left area of the tab
  gfx::ScopedCanvas scoped_canvas(canvas);
  const auto image_size = accent_icon.Size();
  const int x =
      bounds.x() + (BraveTab::kTabAccentIconAreaWidth - image_size.width()) / 2;
  const int y = bounds.y() + (bounds.height() - image_size.height()) / 2;
  canvas->DrawImageInt(accent_icon.Rasterize(tab()->GetColorProvider()), x, y);
}

bool BraveVerticalTabStyle::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowBraveVerticalTabs(
      tab()->controller()->GetBrowserWindowInterface());
}

bool BraveVerticalTabStyle::IsSplitTab(const Tab* tab) const {
  if (!tab) {
    return false;
  }

  return tab->split().has_value();
}

bool BraveVerticalTabStyle::IsStartSplitTab(const Tab* tab) const {
  if (!tab) {
    return false;
  }

  if (!tab->split().has_value()) {
    return false;
  }

  const Tab* tab_to_left = tab->controller()->GetAdjacentTab(tab, -1);
  return std::ranges::none_of(tab->controller()->GetTabsInSplit(tab),
                              [&tab_to_left](const Tab* split_tab) {
                                return split_tab == tab_to_left;
                              });
}

TabStyle::TabColors BraveVerticalTabStyle::CalculateTargetColors() const {
  TabStyle::TabColors colors = TabStyleViewsImpl::CalculateTargetColors();
  std::optional<SkColor> background_color =
      GetTargetTabBackgroundColor(GetSelectionState(), IsHovering());
  if (background_color) {
    colors.background_color = background_color.value();
  }
  return colors;
}

SkColor BraveVerticalTabStyle::GetCurrentTabBackgroundColor(
    TabStyle::TabSelectionState selection_state,
    bool hovered) const {
  std::optional<SkColor> color =
      GetTargetTabBackgroundColor(selection_state, hovered);
  return color.value_or(TabStyleViewsImpl::GetCurrentTabBackgroundColor(
      selection_state, hovered));
}

gfx::RectF BraveVerticalTabStyle::InsetAlignedBoundsForTabAccent(
    const gfx::RectF& bounds,
    float scale) const {
  auto processed_bounds = bounds;
  const auto* brave_tab = static_cast<const BraveTab*>(tab());
  CHECK(brave_tab);
  if (brave_tab->ShouldPaintTabAccent() &&
      brave_tab->ShouldShowLargeAccentIcon()) {
    // Add left inset for tab accent icon area if tab should have accent icon.
    // This will result in GetPath() returning a path with a left insetted by
    // BraveTab::kTabAccentIconAreaWidth * scale.
    processed_bounds.Inset(
        gfx::InsetsF().set_left(BraveTab::kTabAccentIconAreaWidth * scale));
  }

  return processed_bounds;
}

std::optional<SkColor> BraveVerticalTabStyle::GetTargetTabBackgroundColor(
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
  if (IsSplitTab(tab()) && !tab()->IsActive() && !hovered) {
    return SK_ColorTRANSPARENT;
  }

  if (!ShouldShowVerticalTabs()) {
    // Fallback on upstream code.
    return std::nullopt;
  }

  if (tab()->IsActive()) {
    return cp->GetColor(kColorBraveVerticalTabActiveBackground);
  }

  if (hovered) {
    return cp->GetColor(kColorBraveVerticalTabHoveredBackground);
  }

  if (selection_state == TabStyle::TabSelectionState::kSelected) {
    // Use the same color if the tab is selected via multiselection. Fallback on
    // upstream code.
    return std::nullopt;
  }

  return cp->GetColor(kColorBraveVerticalTabInactiveBackground);
}

}  // namespace

std::unique_ptr<TabStyleViews> TabStyleViews::CreateForTab(Tab* tab) {
  return std::make_unique<BraveVerticalTabStyle>(tab);
}
