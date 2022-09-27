/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/views/accessibility/accessibility_paint_checks.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"

namespace {

class ScrollHeaderView : public views::Button {
 public:
  METADATA_HEADER(ScrollHeaderView);

  ScrollHeaderView(Button::PressedCallback callback, TabStrip* tab_strip)
      : Button(std::move(callback)), tab_strip_(tab_strip) {
    // TODO(sangwoo.ko) Temporary workaround before we have a proper tooltip
    // text.
    // https://github.com/brave/brave-browser/issues/24717
    SetProperty(views::kSkipAccessibilityPaintChecks, true);
  }
  ~ScrollHeaderView() override = default;

  // views::Button:
  void OnPaintBackground(gfx::Canvas* canvas) override {
    canvas->DrawColor(GetColorProvider()->GetColor(
        tab_strip_->ShouldPaintAsActiveFrame()
            ? kColorNewTabButtonBackgroundFrameActive
            : kColorNewTabButtonBackgroundFrameInactive));
  }

  void PaintButtonContents(gfx::Canvas* canvas) override {
    // Draw '>'  or '<'
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    // The same color with NewTabButton::GetForegroundColor()
    flags.setColor(tab_strip_->GetTabForegroundColor(TabActive::kInactive));
    flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
    constexpr int kStrokeWidth = 2;
    flags.setStrokeWidth(kStrokeWidth);

    const int icon_width =
        TabStyle::GetPinnedWidth() - TabStyle::GetTabOverlap();
    const bool expanded = icon_width < width();

    const int icon_inset = ui::TouchUiController::Get()->touch_ui() ? 10 : 9;
    gfx::Rect icon_bounds(gfx::Size(icon_width, height()));
    if (expanded)
      icon_bounds.set_x(width() - icon_width);
    icon_bounds.Inset(gfx::Insets::VH(icon_inset, icon_inset * 1.5));

    if (expanded) {
      // '<
      const gfx::Point start = icon_bounds.top_right();
      const gfx::Point mid = {icon_bounds.x(), icon_bounds.CenterPoint().y()};
      const gfx::Point end = icon_bounds.bottom_right();
      canvas->DrawLine(start, mid, flags);
      canvas->DrawLine(mid, end, flags);
    } else {
      // '>'
      const gfx::Point start = icon_bounds.origin();
      const gfx::Point mid = {icon_bounds.right(),
                              icon_bounds.CenterPoint().y()};
      const gfx::Point end = icon_bounds.bottom_left();
      canvas->DrawLine(start, mid, flags);
      canvas->DrawLine(mid, end, flags);
    }
  }

 private:
  raw_ptr<TabStrip> tab_strip_ = nullptr;
};

BEGIN_METADATA(ScrollHeaderView, views::Button)
END_METADATA

// A custom scroll view to avoid crash on Mac
class CustomScrollView : public views::ScrollView {
 public:
  METADATA_HEADER(CustomScrollView);

#if BUILDFLAG(IS_MAC)
  CustomScrollView()
      : views::ScrollView(views::ScrollView::ScrollWithLayers::kDisabled) {}
#else
  CustomScrollView() : views::ScrollView() {}
#endif
  ~CustomScrollView() override = default;

  // views::ScrollView:
  void OnScrollEvent(ui::ScrollEvent* event) override {
#if !BUILDFLAG(IS_MAC)
    views::ScrollView::OnScrollEvent(event);
#endif
  }
};

BEGIN_METADATA(CustomScrollView, views::ScrollView)
END_METADATA

class ScrollContentsView : public views::View {
 public:
  METADATA_HEADER(ScrollContentsView);

  explicit ScrollContentsView(VerticalTabStripRegionView* container)
      : container_(container) {}
  ~ScrollContentsView() override = default;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (height() == GetPreferredSize().height())
      return;

    SetSize({width(), GetPreferredSize().height()});
    container_->Layout();
  }

 private:
  raw_ptr<VerticalTabStripRegionView> container_ = nullptr;
};

BEGIN_METADATA(ScrollContentsView, views::View)
END_METADATA

}  // namespace

VerticalTabStripRegionView::VerticalTabStripRegionView(
    Browser* browser,
    TabStripRegionView* region_view)
    : browser_(browser), region_view_(region_view) {
  scroll_view_ = AddChildView(std::make_unique<CustomScrollView>());
  scroll_view_header_ =
      scroll_view_->SetHeader(std::make_unique<ScrollHeaderView>(
          base::BindRepeating(
              [](VerticalTabStripRegionView* container,
                 const ui::Event& event) {
                container->SetState(
                    container->state_ ==
                            VerticalTabStripRegionView::State::kCollapsed
                        ? VerticalTabStripRegionView::State::kExpanded
                        : VerticalTabStripRegionView::State::kCollapsed);
              },
              this),
          region_view_->tab_strip_));

  auto* scroll_contents =
      scroll_view_->SetContents(std::make_unique<ScrollContentsView>(this));
  scroll_contents->SetLayoutManager(std::make_unique<views::FillLayout>());
  scroll_view_->SetVerticalScrollBarMode(
      views::ScrollView::ScrollBarMode::kHiddenButEnabled);

  new_tab_button_ = AddChildView(std::make_unique<BraveNewTabButton>(
      region_view_->tab_strip_,
      base::BindRepeating(&TabStrip::NewTabButtonPressed,
                          base::Unretained(region_view_->tab_strip_))));
  new_tab_button_->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
  new_tab_button_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));

  UpdateLayout();
}

VerticalTabStripRegionView::~VerticalTabStripRegionView() = default;

bool VerticalTabStripRegionView::IsTabFullscreen() const {
  auto* exclusive_access_manager = browser_->exclusive_access_manager();
  if (!exclusive_access_manager)
    return false;

  auto* fullscreen_controller =
      exclusive_access_manager->fullscreen_controller();
  if (!fullscreen_controller)
    return false;

  return fullscreen_controller->IsWindowFullscreenForTabOrPending();
}

void VerticalTabStripRegionView::SetState(State state) {
  if (state_ == state)
    return;

  state_ = state;
  InvalidateLayout();
}

gfx::Size VerticalTabStripRegionView::CalculatePreferredSize() const {
  if (!tabs::features::ShouldShowVerticalTabs())
    return {};

  if (IsTabFullscreen())
    return {};

  if (state_ == State::kExpanded) {
    return {TabStyle::GetStandardWidth(),
            View::CalculatePreferredSize().height()};
  }

  DCHECK_EQ(state_, State::kCollapsed)
      << "If a new state was added, " << __FUNCTION__
      << " should be revisited.";

  return {TabStyle::GetPinnedWidth() - TabStyle::GetTabOverlap(),
          View::CalculatePreferredSize().height()};
}

void VerticalTabStripRegionView::Layout() {
  // As we have to update ScrollView's viewport size and its contents size,
  // layouting children manually will be more handy.

  // 1. New tab should be fixed at the bottom of container.
  auto contents_bounds = GetContentsBounds();
  new_tab_button_->SetSize(
      {contents_bounds.width(), new_tab_button_->GetPreferredSize().height()});
  new_tab_button_->SetPosition(
      {contents_bounds.x(),
       contents_bounds.bottom() - new_tab_button_->height()});

  // 2. ScrollView takes the rest of space.
  // Set preferred size for scroll view to know this.
  const gfx::Size header_size{scroll_view_->width(),
                              new_tab_button_->GetPreferredSize().height()};
  scroll_view_header_->SetPreferredSize(header_size);
  scroll_view_header_->SetSize(header_size);
  scroll_view_->SetSize({contents_bounds.width(),
                         contents_bounds.height() - new_tab_button_->height()});
  scroll_view_->SetPosition(contents_bounds.origin());
  scroll_view_->ClipHeightTo(0, scroll_view_->height() - header_size.height());

  auto* scroll_view_contents = scroll_view_->contents();
  scroll_view_contents->SetSize(
      {scroll_view_->width(), scroll_view_contents->height()});

  UpdateNewTabButtonVisibility();
  UpdateTabSearchButtonVisibility();
}

void VerticalTabStripRegionView::UpdateLayout() {
  if (tabs::features::ShouldShowVerticalTabs()) {
    if (!Contains(region_view_)) {
      original_parent_of_region_view_ = region_view_->parent();
      original_parent_of_region_view_->RemoveChildView(region_view_);
      scroll_view_->contents()->AddChildView(region_view_.get());
    }
    region_view_->layout_manager_->SetOrientation(
        views::LayoutOrientation::kVertical);
  } else {
    if (Contains(region_view_)) {
      scroll_view_->contents()->RemoveChildView(region_view_);
      original_parent_of_region_view_->AddChildView(region_view_.get());
    }
    region_view_->layout_manager_->SetOrientation(
        views::LayoutOrientation::kHorizontal);
  }
  Layout();
}

void VerticalTabStripRegionView::OnThemeChanged() {
  View::OnThemeChanged();

  auto background_color =
      GetColorProvider()->GetColor(ThemeProperties::COLOR_TOOLBAR);
  SetBackground(views::CreateSolidBackground(background_color));
  scroll_view_->SetBackgroundColor(background_color);

  new_tab_button_->FrameColorsChanged();
}

void VerticalTabStripRegionView::UpdateNewTabButtonVisibility() {
  bool overflowed =
      scroll_view_->GetMaxHeight() < scroll_view_->contents()->height();
  region_view_->new_tab_button()->SetVisible(!overflowed);
  new_tab_button_->SetVisible(overflowed);
}

void VerticalTabStripRegionView::UpdateTabSearchButtonVisibility() {
  const bool is_vertical_tabs = tabs::features::ShouldShowVerticalTabs();
  if (auto* tab_search_button = region_view_->tab_search_button())
    tab_search_button->SetVisible(!is_vertical_tabs);
}

BEGIN_METADATA(VerticalTabStripRegionView, views::View)
END_METADATA
