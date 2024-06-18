/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include <memory>
#include <optional>
#include <utility>

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_slot_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip_observer.h"
#include "chrome/browser/ui/views/tabs/tab_strip_scroll_container.h"
#include "components/tab_groups/tab_group_id.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/views/layout/flex_layout.h"

BraveTabStrip::BraveTabStrip(std::unique_ptr<TabStripController> controller)
    : TabStrip(std::move(controller)) {}

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
  DCHECK(vertical_region_view);

  return vertical_region_view->state() ==
             VerticalTabStripRegionView::State::kFloating ||
         (vertical_region_view->is_animating() &&
          vertical_region_view->last_state() ==
              VerticalTabStripRegionView::State::kFloating &&
          vertical_region_view->state() ==
              VerticalTabStripRegionView::State::kCollapsed);
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
  if (tabs::features::HorizontalTabsUpdateEnabled()) {
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

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
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

  auto new_selection = original_selection;
  if (source->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
    auto* tab = static_cast<Tab*>(source);
    if (auto tile = GetTileForTab(tab)) {
      // Make a pair of tabs in a tile selected together so that they move
      // together during drag and drop.
      auto* tab_strip_model = controller_->GetBrowser()->tab_strip_model();
      // Make sure both tiled tabs are in selection.
      new_selection.AddIndexToSelection(
          tab_strip_model->GetIndexOfTab(tile->first));
      new_selection.AddIndexToSelection(
          tab_strip_model->GetIndexOfTab(tile->second));
      new_selection.set_active(tab_strip_model->active_index());
      tab_strip_model->SetSelectionFromModel(new_selection);
    }
  }

  TabStrip::MaybeStartDrag(source, event, new_selection);
}

void BraveTabStrip::AddedToWidget() {
  TabStrip::AddedToWidget();

  if (BrowserView::GetBrowserViewForBrowser(GetBrowser())) {
    UpdateTabContainer();
  } else {
    // Schedule UpdateTabContainer(). At this point, BrowserWindow could still
    // be being created and it could be unbound to Browser.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&BraveTabStrip::UpdateTabContainer,
                                  weak_factory_.GetWeakPtr()));
  }
}

SkColor BraveTabStrip::GetTabSeparatorColor() const {
  if (ShouldShowVerticalTabs()) {
    return SK_ColorTRANSPARENT;
  }

  Profile* profile = controller()->GetProfile();
  if (!profile->IsRegularProfile()) {
    if (profile->IsTor()) {
      return SkColorSetRGB(0x5A, 0x53, 0x66);
    }

    // For incognito/guest window.
    return SkColorSetRGB(0x3F, 0x32, 0x56);
  }

  // If custom theme is used, follow upstream separator color.
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
  if (theme_service->GetThemeSupplier()) {
    return TabStrip::GetTabSeparatorColor();
  }

  bool dark_mode = dark_mode::GetActiveBraveDarkModeType() ==
                   dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  return dark_mode ? SkColorSetRGB(0x39, 0x38, 0x38)
                   : SkColorSetRGB(0xBE, 0xBF, 0xBF);
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

bool BraveTabStrip::IsTabTiled(const Tab* tab) const {
  return GetTileForTab(tab).has_value();
}

bool BraveTabStrip::IsFirstTabInTile(const Tab* tab) const {
  auto* browser = GetBrowser();
  if (browser->IsBrowserClosing()) {
    return false;
  }

  auto index = tab_container_->GetModelIndexOf(tab);
  if (!index) {
    return false;
  }

  auto tile = GetTileForTab(tab);
  DCHECK(tile) << "This must be called only when IsTabTiled() returned true";
  return browser->tab_strip_model()->GetIndexOfTab(tile->first) == *index;
}

TabTiledState BraveTabStrip::GetTiledStateForTab(int index) const {
  auto* tab = tab_at(index);
  if (!IsTabTiled(tab)) {
    return TabTiledState::kNone;
  }

  return IsFirstTabInTile(tab) ? TabTiledState::kFirst : TabTiledState::kSecond;
}

std::optional<SplitViewBrowserData::Tile> BraveTabStrip::GetTileForTab(
    const Tab* tab) const {
  auto* browser = GetBrowser();
  auto* data = SplitViewBrowserData::FromBrowser(browser);
  if (!data) {
    return std::nullopt;
  }

  if (browser->IsBrowserClosing()) {
    return std::nullopt;
  }
  auto index = tab_container_->GetModelIndexOf(tab);
  if (!index) {
    return std::nullopt;
  }

  if (!browser->tab_strip_model()->ContainsIndex(*index)) {
    // Happens on start-up
    return std::nullopt;
  }

  return data->GetTile(browser->tab_strip_model()->GetTabHandleAt(*index));
}

void BraveTabStrip::UpdateTabContainer() {
  const bool using_vertical_tabs = ShouldShowVerticalTabs();
  const bool should_use_compound_tab_container =
      using_vertical_tabs || base::FeatureList::IsEnabled(tabs::kSplitTabStrip);
  const bool is_using_compound_tab_container =
      views::IsViewClass<BraveCompoundTabContainer>(
          base::to_address(tab_container_));

  base::ScopedClosureRunner layout_lock;
  if (should_use_compound_tab_container != is_using_compound_tab_container) {
    // Before removing any child, we should complete the 'tab closing animation'
    // so that we don't delete views twice.
    tab_container_->CompleteAnimationAndLayout();

    // Resets TabContainer to use.
    auto original_container = RemoveChildViewT(
        static_cast<TabContainer*>(base::to_address(tab_container_)));

    if (should_use_compound_tab_container) {
      // Container should be attached before TabDragContext so that dragged
      // views can be atop container.
      auto* drag_context = GetDragContext();
      auto* brave_tab_container = AddChildViewAt(
          std::make_unique<BraveCompoundTabContainer>(
              *this, hover_card_controller_.get(), drag_context, *this, this),
          0);
      tab_container_ = *brave_tab_container;
      layout_lock =
          base::ScopedClosureRunner(brave_tab_container->LockLayout());

      brave_tab_container->SetScrollEnabled(using_vertical_tabs);

      // Make dragged views on top of container's layer.
      drag_context->SetPaintToLayer();
      drag_context->layer()->SetFillsBoundsOpaquely(false);
      drag_context->parent()->ReorderChildView(drag_context, -1);
    } else {
      // Container should be attached before TabDragContext so that dragged
      // views can be atop container.
      auto* brave_tab_container =
          AddChildViewAt(std::make_unique<BraveTabContainer>(
                             *this, hover_card_controller_.get(),
                             GetDragContext(), *this, this),
                         0);
      tab_container_ = *brave_tab_container;
      layout_lock =
          base::ScopedClosureRunner(brave_tab_container->LockLayout());

      GetDragContext()->DestroyLayer();
    }

    // Resets TabSlotViews for the new TabContainer.
    auto* model = GetBrowser()->tab_strip_model();
    for (int i = 0; i < model->count(); i++) {
      auto* tab = original_container->GetTabAtModelIndex(i);
      // At this point, we don't have group views in the container. So before
      // restoring groups, clears group for tabs.
      tab->set_group(std::nullopt);
      tab_container_->AddTab(
          tab->parent()->RemoveChildViewT(tab), i,
          tab->data().pinned ? TabPinned::kPinned : TabPinned::kUnpinned);
      if (tab->dragging()) {
        GetDragContext()->AddChildView(tab);
      }
    }

    auto* group_model = model->group_model();
    for (auto group_id : group_model->ListTabGroups()) {
      auto* group = group_model->GetTabGroup(group_id);
      tab_container_->OnGroupCreated(group_id);
      const auto tabs = group->ListTabs();
      for (auto i = tabs.start(); i < tabs.end(); i++) {
        AddTabToGroup(group_id, i);
        tab_at(i)->SchedulePaint();
      }
      auto* group_views = tab_container_->GetGroupViews(group_id);
      group_views->UpdateBounds();

      if (auto* original_header =
              original_container->GetGroupViews(group_id)->header();
          original_header->dragging()) {
        group_views->header()->set_dragging(true);
        GetDragContext()->AddChildView(group_views->header());

        group_views->header()->SetBoundsRect(original_header->bounds());
        DCHECK_NE(original_header->parent(), original_container.get())
            << "The header should be child of TabDragContext at this point.";
        original_container->AddChildView(
            original_header->parent()->RemoveChildViewT(original_header));
      }
    }

    for (auto group_id : group_model->ListTabGroups()) {
      auto* visual_data = group_model->GetTabGroup(group_id)->visual_data();
      tab_container_->OnGroupVisualsChanged(group_id, visual_data, visual_data);
    }

    for (int i = 0; i < model->count(); i++) {
      for (auto& observer : observers_) {
        observer.OnTabAdded(i);
      }
    }

    // During drag-and-drop session, the active value could be invalid.
    if (const auto& selection_model = model->selection_model();
        selection_model.active().has_value()) {
      SetSelection(selection_model);
    }
  }

  // Update layout of TabContainer
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
          &VerticalTabStripRegionView::GetAvailableWidthForTabContainer,
          base::Unretained(vertical_region_view)));
    }
  } else {
    if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
      auto* browser_view = static_cast<BraveBrowserView*>(
          BrowserView::GetBrowserViewForBrowser(browser));
      DCHECK(browser_view);
      auto* scroll_container = static_cast<TabStripScrollContainer*>(
          browser_view->tab_strip_region_view()->tab_strip_container_);
      DCHECK(scroll_container);
      SetAvailableWidthCallback(base::BindRepeating(
          &TabStripScrollContainer::GetTabStripAvailableWidth,
          base::Unretained(scroll_container)));
    } else {
      SetAvailableWidthCallback(base::NullCallback());
    }

    if (should_use_compound_tab_container) {
      // Upstream's compound tab container lay out its sub containers manually.
      tab_container_->SetLayoutManager(nullptr);
    }
  }

  hover_card_controller_->SetIsVerticalTabs(using_vertical_tabs);

  if (const auto active_index = GetActiveIndex(); active_index) {
    // In order to update shadow state, call ActiveStateChanged().
    tab_at(active_index.value())->ActiveStateChanged();
  }
}

bool BraveTabStrip::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowVerticalTabs(GetBrowser());
}

TabContainer* BraveTabStrip::GetTabContainerForTesting() {
  return &tab_container_.get();  // IN-TEST
}

void BraveTabStrip::Layout(PassKey) {
  if (ShouldShowVerticalTabs()) {
    // Chromium implementation limits the height of tab strip, which we don't
    // want.
    auto bounds = GetLocalBounds();
    for (views::View* view : children()) {
      if (view->bounds() != bounds) {
        view->SetBoundsRect(GetLocalBounds());
      } else if (view == &tab_container_.get()) {
        view->DeprecatedLayoutImmediately();
      }
    }
    return;
  }

  LayoutSuperclass<TabStrip>(this);
}

BEGIN_METADATA(BraveTabStrip)
END_METADATA
