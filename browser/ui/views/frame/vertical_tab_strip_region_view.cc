/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/views/accessibility/accessibility_paint_checks.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_utils.h"

namespace {

class ToggleButton : public views::Button {
 public:
  METADATA_HEADER(ToggleButton);

  ToggleButton(Button::PressedCallback callback,
               VerticalTabStripRegionView* region_view)
      : Button(std::move(callback)),
        region_view_(region_view),
        tab_strip_(region_view_->tab_strip()) {
    // TODO(sangwoo.ko) Temporary workaround before we have a proper tooltip
    // text.
    // https://github.com/brave/brave-browser/issues/24717
    SetProperty(views::kSkipAccessibilityPaintChecks, true);
    SetPreferredSize(gfx::Size{tabs::kVerticalTabMinWidth, GetIconWidth()});
  }
  ~ToggleButton() override = default;

  static int GetIconWidth() {
    return TabStyle::GetPinnedWidth() - TabStyle::GetTabOverlap();
  }

  // views::Button:
  void PaintButtonContents(gfx::Canvas* canvas) override {
    // Draw '>'  or '<'
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    // The same color with NewTabButton::GetForegroundColor()
    flags.setColor(tab_strip_->GetTabForegroundColor(TabActive::kInactive));
    flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
    constexpr int kStrokeWidth = 2;
    flags.setStrokeWidth(kStrokeWidth);

    const int icon_width = GetIconWidth();

    const bool expanded =
        region_view_->state() == VerticalTabStripRegionView::State::kExpanded;

    const int icon_inset = ui::TouchUiController::Get()->touch_ui() ? 10 : 9;
    gfx::Rect icon_bounds(gfx::Size(icon_width, height()));
    icon_bounds.set_x((width() - icon_width) / 2);
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
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;
};

BEGIN_METADATA(ToggleButton, views::Button)
END_METADATA

class ScrollHeaderView : public views::View {
 public:
  METADATA_HEADER(ScrollHeaderView);

  ScrollHeaderView(views::Button::PressedCallback toggle_callback,
                   VerticalTabStripRegionView* region_view)
      : region_view_(region_view), tab_strip_(region_view->tab_strip()) {
    auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal));

    AddChildView(std::make_unique<ToggleButton>(std::move(toggle_callback),
                                                region_view));

    auto* spacer = AddChildView(std::make_unique<views::View>());
    layout->SetFlexForView(spacer,
                           1 /* resize |spacer| to fill the rest of space */);

    // We layout the search button at the end, because there's no
    // way to change its bubble arrow from TOP_RIGHT at the moment.
    tab_search_button_ = AddChildView(
        std::make_unique<BraveTabSearchButton>(region_view->tab_strip()));
    tab_search_button_->SetPreferredSize(
        gfx::Size{ToggleButton::GetIconWidth(), ToggleButton::GetIconWidth()});
    tab_search_button_->SetTooltipText(
        l10n_util::GetStringUTF16(IDS_TOOLTIP_TAB_SEARCH));
    tab_search_button_->SetAccessibleName(
        l10n_util::GetStringUTF16(IDS_ACCNAME_TAB_SEARCH));

    UpdateTabSearchButtonVisibility();
  }
  ~ScrollHeaderView() override = default;

  void UpdateTabSearchButtonVisibility() {
    tab_search_button_->SetVisible(
        region_view_->state() !=
            VerticalTabStripRegionView::State::kCollapsed &&
        !WindowFrameUtil::IsWin10TabSearchCaptionButtonEnabled(
            region_view_->browser()));
  }

  // views::View:
  void OnThemeChanged() override {
    View::OnThemeChanged();
    tab_search_button_->FrameColorsChanged();
  }

  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    View::OnBoundsChanged(previous_bounds);
    UpdateTabSearchButtonVisibility();
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    canvas->DrawColor(GetColorProvider()->GetColor(
        tab_strip_->ShouldPaintAsActiveFrame()
            ? kColorNewTabButtonBackgroundFrameActive
            : kColorNewTabButtonBackgroundFrameInactive));
  }

 private:
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;
  raw_ptr<const TabStrip> tab_strip_ = nullptr;
  raw_ptr<TabSearchButton> tab_search_button_ = nullptr;
};

BEGIN_METADATA(ScrollHeaderView, views::View)
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

  ScrollContentsView(VerticalTabStripRegionView* container, TabStrip* tab_strip)
      : container_(container), tab_strip_(tab_strip) {}
  ~ScrollContentsView() override = default;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip))
      return;

    container_->UpdateNewTabButtonVisibility();
    if (height() == GetPreferredSize().height())
      return;

    SetSize({width(), GetPreferredSize().height()});
    container_->Layout();
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    canvas->DrawColor(GetColorProvider()->GetColor(
        tab_strip_->ShouldPaintAsActiveFrame()
            ? kColorNewTabButtonBackgroundFrameActive
            : kColorNewTabButtonBackgroundFrameInactive));
  }

 private:
  raw_ptr<VerticalTabStripRegionView> container_ = nullptr;
  raw_ptr<TabStrip> tab_strip_ = nullptr;
};

BEGIN_METADATA(ScrollContentsView, views::View)
END_METADATA

}  // namespace

VerticalTabStripRegionView::VerticalTabStripRegionView(
    Browser* browser,
    TabStripRegionView* region_view)
    : browser_(browser), region_view_(region_view) {
  SetNotifyEnterExitOnChild(true);

  scroll_view_ = AddChildView(std::make_unique<CustomScrollView>());
  scroll_view_header_ =
      scroll_view_->SetHeader(std::make_unique<ScrollHeaderView>(
          base::BindRepeating(
              [](VerticalTabStripRegionView* container,
                 const ui::Event& event) {
                // Note that Calling SetValue() doesn't trigger
                // OnCollapsedPrefChanged() for this view.
                if (container->state_ == State::kExpanded) {
                  container->collapsed_pref_.SetValue(true);
                  container->SetState(State::kCollapsed);
                } else {
                  container->collapsed_pref_.SetValue(false);
                  container->SetState(State::kExpanded);
                }
              },
              this),
          this));

  scroll_contents_view_ = scroll_view_->SetContents(
      std::make_unique<ScrollContentsView>(this, region_view_->tab_strip_));
  scroll_contents_view_->SetLayoutManager(
      std::make_unique<views::FillLayout>());
  scroll_view_->SetVerticalScrollBarMode(
      base::FeatureList::IsEnabled(features::kScrollableTabStrip)
          ? views::ScrollView::ScrollBarMode::kDisabled
          : views::ScrollView::ScrollBarMode::kHiddenButEnabled);

  new_tab_button_ = AddChildView(std::make_unique<BraveNewTabButton>(
      region_view_->tab_strip_,
      base::BindRepeating(&TabStrip::NewTabButtonPressed,
                          base::Unretained(region_view_->tab_strip_))));
  new_tab_button_->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
  new_tab_button_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));

  auto* prefs = browser_->profile()->GetOriginalProfile()->GetPrefs();
  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::UpdateLayout,
                          base::Unretained(this), false));
  UpdateLayout();

  collapsed_pref_.Init(
      brave_tabs::kVerticalTabsCollapsed, prefs,
      base::BindRepeating(&VerticalTabStripRegionView::OnCollapsedPrefChanged,
                          base::Unretained(this)));
  OnCollapsedPrefChanged();

  floating_mode_pref_.Init(
      brave_tabs::kVerticalTabsFloatingEnabled, prefs,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnFloatingModePrefChanged,
          base::Unretained(this)));
  OnFloatingModePrefChanged();
}

VerticalTabStripRegionView::~VerticalTabStripRegionView() {
  // We need to move tab strip region to its original parent to avoid crash
  // during drag and drop session.
  UpdateLayout(true);
}

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

  mouse_enter_timer_.Stop();
  state_ = state;

  region_view_->tab_strip_->SetAvailableWidthCallback(base::BindRepeating(
      &VerticalTabStripRegionView::GetPreferredWidthForState,
      base::Unretained(this), state_));
  region_view_->tab_strip_->tab_container_->InvalidateIdealBounds();

  PreferredSizeChanged();
}

VerticalTabStripRegionView::ScopedStateResetter
VerticalTabStripRegionView::ExpandTabStripForDragging() {
  if (state_ == State::kExpanded)
    return {};

  auto resetter = std::make_unique<base::ScopedClosureRunner>(
      base::BindOnce(&VerticalTabStripRegionView::SetState,
                     weak_factory_.GetWeakPtr(), State::kCollapsed));

  SetState(State::kExpanded);
  // In this case, we dont' wait for the widget bounds to be changed so that
  // tab drag controller can layout tabs properly.
  SetSize(GetPreferredSize());

  return resetter;
}

gfx::Vector2d VerticalTabStripRegionView::GetOffsetForDraggedTab() const {
  return {0, scroll_view_header_->GetPreferredSize().height()};
}

int VerticalTabStripRegionView::GetAvailableWidthForTabContainer() {
  return GetPreferredWidthForState(state_);
}

gfx::Size VerticalTabStripRegionView::CalculatePreferredSize() const {
  return GetPreferredSizeForState(state_);
}

gfx::Size VerticalTabStripRegionView::GetMinimumSize() const {
  if (state_ == State::kFloating)
    return GetPreferredSizeForState(State::kCollapsed);

  return GetPreferredSizeForState(state_);
}

void VerticalTabStripRegionView::Layout() {
  if (size() == last_layout_size_)
    return;

  last_layout_size_ = size();

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
  const gfx::Size header_size{contents_bounds.width(),
                              new_tab_button_->GetPreferredSize().height()};
  scroll_view_header_->SetPreferredSize(header_size);
  scroll_view_header_->SetSize(header_size);

  scroll_view_->SetSize({contents_bounds.width(),
                         contents_bounds.height() - new_tab_button_->height()});
  scroll_view_->SetPosition(contents_bounds.origin());
  auto scroll_contents_height = scroll_view_->height() - header_size.height();
  scroll_view_->ClipHeightTo(0, scroll_contents_height);

  if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
    scroll_contents_view_->SetSize(
        {scroll_view_->width(), scroll_view_->height()});
    auto* nested_scroll_view = GetTabStripScrollContainer()->scroll_view_.get();
    nested_scroll_view->SetSize(
        {scroll_view_->width(), scroll_contents_height});
    nested_scroll_view->ClipHeightTo(0, scroll_contents_height);
  } else {
    scroll_contents_view_->SetSize(
        {scroll_view_->width(), scroll_contents_height});
  }

  UpdateNewTabButtonVisibility();
  UpdateTabSearchButtonVisibility();
}

void VerticalTabStripRegionView::UpdateLayout(bool in_destruction) {
  if (tabs::features::ShouldShowVerticalTabs(browser_) && !in_destruction) {
    if (!Contains(region_view_)) {
      original_parent_of_region_view_ = region_view_->parent();
      original_parent_of_region_view_->RemoveChildView(region_view_);
      scroll_view_->contents()->AddChildView(region_view_.get());
    }

    region_view_->layout_manager_->SetOrientation(
        views::LayoutOrientation::kVertical);
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
      auto* scroll_container = GetTabStripScrollContainer();
      scroll_container->SetLayoutManager(std::make_unique<views::FillLayout>());
      scroll_container->scroll_view_->SetTreatAllScrollEventsAsHorizontal(
          false);
      scroll_container->scroll_view_->SetVerticalScrollBarMode(
          views::ScrollView::ScrollBarMode::kHiddenButEnabled);
      scroll_container->overflow_view_->SetOrientation(
          views::LayoutOrientation::kVertical);
    }
  } else {
    if (Contains(region_view_)) {
      scroll_view_->contents()->RemoveChildView(region_view_);
      // TabStrip should be added before other views so that we can preserve
      // the z-order. At this moment, tab strip is the first child of the
      // parent view.
      // https://github.com/chromium/chromium/blob/bdcef78b63f64119bbe950386b2495a045629f0e/chrome/browser/ui/views/frame/browser_view.cc#L904
      original_parent_of_region_view_->AddChildViewAt(region_view_.get(), 0);
    }

    region_view_->layout_manager_->SetOrientation(
        views::LayoutOrientation::kHorizontal);
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
      auto* scroll_container = GetTabStripScrollContainer();
      scroll_container->SetLayoutManager(std::make_unique<views::FillLayout>())
          ->SetMinimumSizeEnabled(true);
      scroll_container->scroll_view_->SetTreatAllScrollEventsAsHorizontal(true);
      scroll_container->scroll_view_->SetVerticalScrollBarMode(
          views::ScrollView::ScrollBarMode::kDisabled);
      scroll_container->overflow_view_->SetOrientation(
          views::LayoutOrientation::kHorizontal);
    }
  }

  PreferredSizeChanged();
  last_layout_size_ = {};
  Layout();
}

void VerticalTabStripRegionView::OnThemeChanged() {
  View::OnThemeChanged();
  scroll_view_->SetBackgroundColor(GetColorProvider()->GetColor(kColorToolbar));

  new_tab_button_->FrameColorsChanged();
}

void VerticalTabStripRegionView::OnMouseExited(const ui::MouseEvent& event) {
  if (IsMouseHovered() && !mouse_events_for_test_) {
    // On Windows, when mouse moves into the area which intersects with web
    // view, OnMouseExited() is invoked even mouse is on this view.
    return;
  }

  mouse_enter_timer_.Stop();
  if (state_ == State::kFloating)
    SetState(State::kCollapsed);
}

void VerticalTabStripRegionView::OnMouseEntered(const ui::MouseEvent& event) {
  if (!tabs::features::IsFloatingVerticalTabsEnabled(browser_))
    return;

  // During tab dragging, this could be already expanded.
  if (state_ == State::kExpanded)
    return;

  ScheduleFloatingModeTimer();
}

void VerticalTabStripRegionView::UpdateNewTabButtonVisibility() {
  bool overflowed =
      tabs::features::ShouldShowVerticalTabs(browser_) &&
      scroll_view_->GetMaxHeight() < scroll_view_->contents()->height();
  region_view_->new_tab_button()->SetVisible(!overflowed);
  new_tab_button_->SetVisible(overflowed);
}

void VerticalTabStripRegionView::UpdateTabSearchButtonVisibility() {
  const bool is_vertical_tabs =
      tabs::features::ShouldShowVerticalTabs(browser_);
  if (auto* tab_search_button = region_view_->tab_search_button())
    tab_search_button->SetVisible(!is_vertical_tabs);
}

void VerticalTabStripRegionView::OnCollapsedPrefChanged() {
  SetState(collapsed_pref_.GetValue() ? State::kCollapsed : State::kExpanded);
}

void VerticalTabStripRegionView::OnFloatingModePrefChanged() {
  if (!tabs::features::IsFloatingVerticalTabsEnabled(browser_)) {
    if (state_ == State::kFloating)
      SetState(State::kCollapsed);
    return;
  }

  if (IsMouseHovered())
    ScheduleFloatingModeTimer();
}

gfx::Size VerticalTabStripRegionView::GetPreferredSizeForState(
    State state) const {
  if (!tabs::features::ShouldShowVerticalTabs(browser_))
    return {};

  if (IsTabFullscreen())
    return {};

  return {GetPreferredWidthForState(state),
          View::CalculatePreferredSize().height()};
}

int VerticalTabStripRegionView::GetPreferredWidthForState(State state) const {
  if (state == State::kExpanded || state == State::kFloating)
    return TabStyle::GetStandardWidth();

  DCHECK_EQ(state, State::kCollapsed)
      << "If a new state was added, " << __FUNCTION__
      << " should be revisited.";
  return tabs::kVerticalTabMinWidth;
}

TabStripScrollContainer*
VerticalTabStripRegionView::GetTabStripScrollContainer() {
  DCHECK(base::FeatureList::IsEnabled(features::kScrollableTabStrip));
  auto* scroll_container = views::AsViewClass<TabStripScrollContainer>(
      region_view_->tab_strip_container_);
  DCHECK(scroll_container)
      << "TabStripScrollContainer is used by upstream at this moment.";
  return scroll_container;
}

void VerticalTabStripRegionView::ScheduleFloatingModeTimer() {
  if (mouse_events_for_test_) {
    SetState(State::kFloating);
    return;
  }

  mouse_enter_timer_.Stop();
  if (state_ == State::kCollapsed) {
    mouse_enter_timer_.Start(
        FROM_HERE, base::Milliseconds(400),
        base::BindOnce(&VerticalTabStripRegionView::SetState,
                       base::Unretained(this), State::kFloating));
  }
}

BEGIN_METADATA(VerticalTabStripRegionView, views::View)
END_METADATA
