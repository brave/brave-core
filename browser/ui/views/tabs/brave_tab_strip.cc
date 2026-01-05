/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_slot_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_observer.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/tab_group.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/layout/flex_layout.h"

BraveTabStrip::BraveTabStrip(std::unique_ptr<TabStripController> controller)
    : TabStrip(std::move(controller)) {
  always_hide_close_button_.Init(
      brave_tabs::kAlwaysHideTabCloseButton,
      controller_->GetProfile()->GetPrefs(),
      base::BindRepeating(&BraveTabStrip::OnAlwaysHideCloseButtonPrefChanged,
                          base::Unretained(this)));
}

BraveTabStrip::~BraveTabStrip() = default;

bool BraveTabStrip::IsVerticalTabsFloating() const {
  if (!ShouldShowVerticalTabs()) {
    // Can happen when switching the orientation
    return false;
  }

  auto* browser = GetBrowser();
  DCHECK(browser);
  auto* browser_view = static_cast<BraveBrowserView*>(
      BrowserView::GetBrowserViewForBrowser(browser));
  if (!browser_view) {
    // Could be null during the start-up.
    return false;
  }

  auto* vertical_region_view =
      browser_view->vertical_tab_strip_widget_delegate_view()
          ->vertical_tab_strip_region_view();

  if (!vertical_region_view) {
    // Could be null while closing a window.
    return false;
  }

  return vertical_region_view->state() ==
             BraveVerticalTabStripRegionView::State::kFloating ||
         (vertical_region_view->is_animating() &&
          vertical_region_view->last_state() ==
              BraveVerticalTabStripRegionView::State::kFloating &&
          vertical_region_view->state() ==
              BraveVerticalTabStripRegionView::State::kCollapsed);
}

bool BraveTabStrip::CanPaintThrobberToLayer() const {
  if (!ShouldShowVerticalTabs()) {
    return TabStrip::CanPaintThrobberToLayer();
  }

  // Don't allow throbber to be painted to layer. Vertical tabs are scrollable,
  // and a tab could be out of the viewport. Otherwise, throbber would be
  // painted even when the tab is not in the viewport.
  return false;
}

bool BraveTabStrip::ShouldDrawStrokes() const {
  if (ShouldShowVerticalTabs()) {
    // Prevent root view from drawing lines. For vertical tabs stroke , we
    // ignore this method and always draw strokes in GetStrokeThickness().
    return false;
  }

  // TODO(simonhong): We can return false always here as horizontal tab design
  // doesn't need additional stroke.
  // Delete all below code when horizontal tab feature flag is removed.
  if (tabs::HorizontalTabsUpdateEnabled()) {
    // We never automatically draw strokes around tabs. For pinned tabs, we draw
    // the stroke when generating the tab drawing path.
    return false;
  }

  if (!TabStrip::ShouldDrawStrokes()) {
    return false;
  }

  // Use a little bit lower minimum contrast ratio as our ratio is 1.08162
  // between default tab background and frame color of light theme.
  // With upstream's 1.3f minimum ratio, strokes are drawn and it causes weird
  // border lines in the tab group.
  // Set 1.0816f as a minimum ratio to prevent drawing stroke.
  // We don't need the stroke for our default light theme.
  // NOTE: We don't need to check features::kTabOutlinesInLowContrastThemes
  // enabled state. Although TabStrip::ShouldDrawStrokes() has related code,
  // that feature is already expired since cr82. See
  // chrome/browser/flag-metadata.json.
  const SkColor background_color = TabStyle::Get()->GetTabBackgroundColor(
      TabStyle::TabSelectionState::kActive, /*hovered=*/false,
      /*frame_active*/ true, *GetColorProvider());
  const SkColor frame_color =
      controller_->GetFrameColor(BrowserFrameActiveState::kActive);
  const float contrast_ratio =
      color_utils::GetContrastRatio(background_color, frame_color);
  return contrast_ratio < kBraveMinimumContrastRatioForOutlines;
}

void BraveTabStrip::ShowHover(Tab* tab, TabStyle::ShowHoverStyle style) {
  // Chromium asks hover style to all split tabs but we only set hover style
  // to hovered tab.
  tab->ShowHover(style);
}

void BraveTabStrip::HideHover(Tab* tab, TabStyle::HideHoverStyle style) {
  // See the comment of ShowHover().
  tab->HideHover(style);
}

void BraveTabStrip::UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) {
  if (brave_tabs::AreTooltipsEnabled(controller_->GetProfile()->GetPrefs())) {
    return;
  }
  TabStrip::UpdateHoverCard(tab, update_type);
}

void BraveTabStrip::MaybeStartDrag(
    TabSlotView* source,
    const ui::LocatedEvent& event,
    const ui::ListSelectionModel& original_selection) {
  if (ShouldShowVerticalTabs()) {
    // When it's vertical tab strip, all the dragged tabs are either pinned or
    // unpinned.
    const bool source_is_pinned =
        source->GetTabSlotViewType() == TabSlotView::ViewType::kTab &&
        static_cast<Tab*>(source)->data().pinned;
    for (size_t index : original_selection.selected_indices()) {
      if (controller_->IsTabPinned(index) != source_is_pinned) {
        return;
      }
    }
  }

  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
    // When source tab is bound for dummy web contents for a shared pinned tab,
    // we shouldn't kick off drag-and-drop session as the web contents will be
    // replaced soon.
    if (source->GetTabSlotViewType() == TabSlotView::ViewType::kTab &&
        static_cast<Tab*>(source)->data().pinned) {
      auto index = GetModelIndexOf(source).value();
      auto* browser = controller_->GetBrowser();
      DCHECK(browser);

      auto* shared_pinned_tab_service =
          SharedPinnedTabServiceFactory::GetForProfile(browser->profile());
      DCHECK(shared_pinned_tab_service);
      if (shared_pinned_tab_service->IsDummyContents(
              browser->tab_strip_model()->GetWebContentsAt(index))) {
        return;
      }
    }
  }

  TabStrip::MaybeStartDrag(source, event, original_selection);
}

void BraveTabStrip::AddedToWidget() {
  TabStrip::AddedToWidget();

  if (BrowserView::GetBrowserViewForBrowser(GetBrowser())) {
    UpdateOrientation();
  } else {
    // Schedule UpdateOrientation(). At this point, BrowserWindow could still
    // be being created and it could be unbound to Browser.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&BraveTabStrip::UpdateOrientation,
                                  weak_factory_.GetWeakPtr()));
  }
}

std::optional<int> BraveTabStrip::GetCustomBackgroundId(
    BrowserFrameActiveState active_state) const {
  if (!ShouldShowVerticalTabs()) {
    return TabStrip::GetCustomBackgroundId(active_state);
  }

  // When vertical tab strip mode is enabled, the tab strip could be reattached
  // to the original parent during destruction. In this case, theme changing
  // could occur. But unfortunately, some of native widget's implementation
  // doesn't check the validity of pointer, which causes crash.
  // e.g. DesktopNativeWidgetAura's many methods desktop_tree_host without
  //      checking it's validity.
  // In order to avoid accessing invalid pointer, filters here.
  if (auto* widget = GetWidget();
      !widget || widget->IsClosed() || !widget->native_widget()) {
    return {};
  }

  return TabStrip::GetCustomBackgroundId(active_state);
}

void BraveTabStrip::SetCustomTitleForTab(
    Tab* tab,
    const std::optional<std::u16string>& title) {
  auto index = GetModelIndexOf(tab);
  CHECK(index);

  static_cast<BraveBrowserTabStripController*>(controller_.get())
      ->SetCustomTitleForTab(*index, title);
}

bool BraveTabStrip::ShouldAlwaysHideCloseButton() const {
  return *always_hide_close_button_;
}

void BraveTabStrip::EnterTabRenameModeAt(int index) {
  auto* tab = tab_at(index);
  CHECK(tab);
  static_cast<BraveTab*>(tab)->EnterRenameMode();
}

bool BraveTabStrip::ShouldShowPinnedTabsInGrid() const {
  // Basically we don't want to layout pinned tabs in grid when vertical tabs
  // are floating. Otherwise, pinned tabs would jump to the top of tab strip
  // when mouse hovers over the pinned tabs, and requires extra mouse movement
  // to reach the desired tab.
  bool should_layout_pinned_tabs_in_grid = !IsVerticalTabsFloating();

  if (!should_layout_pinned_tabs_in_grid &&
      base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely)) {
    // Even when in floating mode, we layout pinned tabs in a grid when
    // "Hide Completely When Collapsed" is enabled. In this case, pinned tabs
    // are not visible at all, so we don't need to care about the jumping issue.
    should_layout_pinned_tabs_in_grid =
        controller_->GetProfile()->GetPrefs()->GetBoolean(
            brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed);
  }

  return should_layout_pinned_tabs_in_grid;
}

void BraveTabStrip::UpdateOrientation() {
  const bool using_vertical_tabs = ShouldShowVerticalTabs();
  auto* browser = GetBrowser();
  DCHECK(browser);

  if (using_vertical_tabs) {
    auto* browser_view = static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForBrowser(browser));
    DCHECK(browser_view);
    auto* vertical_region_view =
        browser_view->vertical_tab_strip_widget_delegate_view()
            ->vertical_tab_strip_region_view();
    // `vertical_region_view` can be null if it's in destruction.
    if (vertical_region_view) {
      SetAvailableWidthCallback(base::BindRepeating(
          &BraveVerticalTabStripRegionView::GetAvailableWidthForTabContainer,
          base::Unretained(vertical_region_view)));
    }
  } else {
    SetAvailableWidthCallback(base::NullCallback());
  }

  hover_card_controller_->SetIsVerticalTabs(using_vertical_tabs);

  if (const auto active_index = GetActiveIndex(); active_index) {
    // In order to update shadow state, call ActiveStateChanged().
    tab_at(active_index.value())->ActiveStateChanged();
  }

  // Called only at startup or vertical tab mode changed.
  // To make initial tabs layout properly.
  DeprecatedLayoutImmediately();
}

bool BraveTabStrip::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowVerticalTabs(GetBrowser());
}

void BraveTabStrip::OnAlwaysHideCloseButtonPrefChanged() {
  // Invalidate layout of all tabs to update close button visibility.
  // The visibility of close button is updated in Tab::Layout().
  for (int i = 0; i < GetTabCount(); ++i) {
    tab_at(i)->InvalidateLayout();
  }
}

TabContainer* BraveTabStrip::GetTabContainerForTesting() {
  return &tab_container_.get();  // IN-TEST
}

BEGIN_METADATA(BraveTabStrip)
END_METADATA
