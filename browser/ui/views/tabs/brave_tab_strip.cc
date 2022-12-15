/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/flex_layout.h"

BraveTabStrip::BraveTabStrip(std::unique_ptr<TabStripController> controller)
    : TabStrip(std::move(controller)) {}

BraveTabStrip::~BraveTabStrip() = default;

bool BraveTabStrip::ShouldDrawStrokes() const {
  if (tabs::features::ShouldShowVerticalTabs(GetBrowser()))
    return false;

  if (!TabStrip::ShouldDrawStrokes())
    return false;

  // Use a little bit lower minimum contrast ratio as our ratio is 1.27979
  // between default tab background and frame color of light theme.
  // With upstream's 1.3f minimum ratio, strokes are drawn and it causes weird
  // border lines in the tab group.
  // Set 1.2797f as a minimum ratio to prevent drawing stroke.
  // We don't need the stroke for our default light theme.
  // NOTE: We don't need to check features::kTabOutlinesInLowContrastThemes
  // enabled state. Althought TabStrip::ShouldDrawStrokes() has related code,
  // that feature is already expired since cr82. See
  // chrome/browser/flag-metadata.json.
  const SkColor background_color = GetTabBackgroundColor(
      TabActive::kActive, BrowserFrameActiveState::kActive);
  const SkColor frame_color =
      controller_->GetFrameColor(BrowserFrameActiveState::kActive);
  const float contrast_ratio =
      color_utils::GetContrastRatio(background_color, frame_color);
  return contrast_ratio < kBraveMinimumContrastRatioForOutlines;
}

void BraveTabStrip::UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) {
  if (brave_tabs::AreTooltipsEnabled(controller_->GetProfile()->GetPrefs()))
    return;
  TabStrip::UpdateHoverCard(tab, update_type);
}

void BraveTabStrip::MaybeStartDrag(
    TabSlotView* source,
    const ui::LocatedEvent& event,
    const ui::ListSelectionModel& original_selection) {
  if (tabs::features::ShouldShowVerticalTabs(GetBrowser())) {
    // When it's vertical tab strip, all the dragged tabs are either pinned or
    // unpinned.
    const bool source_is_pinned =
        source->GetTabSlotViewType() == TabSlotView::ViewType::kTab &&
        static_cast<Tab*>(source)->data().pinned;
    for (size_t index : original_selection.selected_indices()) {
      if (controller_->IsTabPinned(index) != source_is_pinned)
        return;
    }
  }

  TabStrip::MaybeStartDrag(source, event, original_selection);
}

void BraveTabStrip::AddedToWidget() {
  TabStrip::AddedToWidget();

  if (BrowserView::GetBrowserViewForBrowser(GetBrowser())) {
    UpdateTabContainer();
  } else {
    // Schedule UpdateTabContainer(). At this point, BrowserWindow could still
    // be being created and it could be unbound to Browser.
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&BraveTabStrip::UpdateTabContainer,
                                  base::Unretained(this)));
  }
}

SkColor BraveTabStrip::GetTabSeparatorColor() const {
  if (tabs::features::ShouldShowVerticalTabs(GetBrowser()))
    return SK_ColorTRANSPARENT;

  Profile* profile = controller()->GetProfile();
  if (!brave::IsRegularProfile(profile)) {
    if (profile->IsTor())
      return SkColorSetRGB(0x5A, 0x53, 0x66);

    // For incognito/guest window.
    return SkColorSetRGB(0x3F, 0x32, 0x56);
  }

  // If custom theme is used, follow upstream separator color.
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
  if (theme_service->GetThemeSupplier())
    return TabStrip::GetTabSeparatorColor();

  bool dark_mode = dark_mode::GetActiveBraveDarkModeType() ==
                   dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  return dark_mode ? SkColorSetRGB(0x39, 0x38, 0x38)
                   : SkColorSetRGB(0xBE, 0xBF, 0xBF);
}

void BraveTabStrip::UpdateTabContainer() {
  auto* browser = GetBrowser();
  DCHECK(browser);
  const bool using_vertical_tabs =
      tabs::features::ShouldShowVerticalTabs(browser);
  const bool should_use_compound_tab_container =
      using_vertical_tabs ||
      base::FeatureList::IsEnabled(features::kSplitTabStrip);
  const bool is_using_compound_tab_container =
      tab_container_->GetClassName() ==
      BraveCompoundTabContainer::kViewClassName;
  base::ScopedClosureRunner layout_lock;
  if (should_use_compound_tab_container != is_using_compound_tab_container) {
    // Resets TabContainer to use.
    RemoveChildViewT(
        static_cast<views::View*>(base::to_address(tab_container_)));

    if (should_use_compound_tab_container) {
      auto* brave_tab_container =
          AddChildView(std::make_unique<BraveCompoundTabContainer>(
              raw_ref<TabContainerController>::from_ptr(this),
              hover_card_controller_.get(), GetDragContext(), *this, this));
      tab_container_ = *brave_tab_container;
      layout_lock =
          base::ScopedClosureRunner(brave_tab_container->LockLayout());
    } else {
      auto* brave_tab_container =
          AddChildView(std::make_unique<BraveTabContainer>(
              *this, hover_card_controller_.get(), GetDragContext(), *this,
              this));
      tab_container_ = *brave_tab_container;
      layout_lock =
          base::ScopedClosureRunner(brave_tab_container->LockLayout());
    }

    // Resets TabSlotViews for the new TabContainer.
    base::flat_set<tab_groups::TabGroupId> groups;
    auto* model = GetBrowser()->tab_strip_model();
    for (int i = 0; i < model->count(); i++) {
      auto data = TabRendererData::FromTabInModel(model, i);
      const bool pinned = data.pinned;
      auto* tab = tab_container_->AddTab(
          std::make_unique<BraveTab>(this), i,
          pinned ? TabPinned::kPinned : TabPinned::kUnpinned);

      tab->set_context_menu_controller(&context_menu_controller_);
      tab->AddObserver(this);

      tab->SetData(std::move(data));
    }

    auto* group_model = model->group_model();
    for (auto group_id : group_model->ListTabGroups()) {
      auto* group = group_model->GetTabGroup(group_id);
      tab_container_->OnGroupCreated(group_id);
      const auto tabs = group->ListTabs();
      for (auto i = tabs.start(); i < tabs.end(); i++)
        AddTabToGroup(group_id, i);

      auto* visual_data = group->visual_data();
      tab_container_->OnGroupVisualsChanged(group_id, visual_data, visual_data);
    }
  }

  // Update layout of TabContainer
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

    tab_container_->SetLayoutManager(std::make_unique<views::FlexLayout>())
        ->SetOrientation(views::LayoutOrientation::kVertical);
  } else {
    SetAvailableWidthCallback(base::NullCallback());
    if (should_use_compound_tab_container) {
      tab_container_->SetLayoutManager(std::make_unique<views::FlexLayout>())
          ->SetOrientation(views::LayoutOrientation::kVertical);
    }
  }
}

void BraveTabStrip::Layout() {
  if (tabs::features::ShouldShowVerticalTabs(GetBrowser()) &&
      base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
    // Chromium implementation limits the height of tab strip, which we don't
    // want.
    auto bounds = GetLocalBounds();
    for (auto* view : children()) {
      if (view->bounds() != bounds)
        view->SetBoundsRect(GetLocalBounds());
      else if (view == &tab_container_.get())
        view->Layout();
    }
    return;
  }

  TabStrip::Layout();
}

BEGIN_METADATA(BraveTabStrip, TabStrip)
END_METADATA
