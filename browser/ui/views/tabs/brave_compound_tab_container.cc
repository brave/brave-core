/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/ui_features.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_utils.h"

BraveCompoundTabContainer::BraveCompoundTabContainer(
    TabContainerController& controller,
    TabHoverCardController* hover_card_controller,
    TabDragContextBase* drag_context,
    TabSlotController& tab_slot_controller,
    views::View* scroll_contents_view)
    : CompoundTabContainer(controller,
                           hover_card_controller,
                           drag_context,
                           tab_slot_controller,
                           scroll_contents_view),
      tab_slot_controller_(tab_slot_controller) {}

void BraveCompoundTabContainer::SetAvailableWidthCallback(
    base::RepeatingCallback<int()> available_width_callback) {
  CompoundTabContainer::SetAvailableWidthCallback(available_width_callback);
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return;

  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser()) &&
      available_width_callback) {
    pinned_tab_container_->SetAvailableWidthCallback(
        base::BindRepeating(&views::View::width, base::Unretained(this)));
    unpinned_tab_container_->SetAvailableWidthCallback(
        base::BindRepeating(&views::View::width, base::Unretained(this)));
    return;
  }

  // Upstream's compound tab container doesn't use this.
  pinned_tab_container_->SetAvailableWidthCallback(base::NullCallback());
  unpinned_tab_container_->SetAvailableWidthCallback(base::NullCallback());
}

BraveCompoundTabContainer::~BraveCompoundTabContainer() = default;

base::OnceClosure BraveCompoundTabContainer::LockLayout() {
  std::vector<base::OnceClosure> closures;
  for (const auto& tab_container :
       {unpinned_tab_container_, pinned_tab_container_}) {
    closures.push_back(
        static_cast<BraveTabContainer*>(base::to_address(tab_container))
            ->LockLayout());
  }

  return base::BindOnce(
      [](std::vector<base::OnceClosure> closures) {
        base::ranges::for_each(closures,
                               [](auto& closure) { std::move(closure).Run(); });
      },
      std::move(closures));
}

void BraveCompoundTabContainer::TransferTabBetweenContainers(
    int from_model_index,
    int to_model_index) {
  const bool was_pinned = to_model_index < NumPinnedTabs();
  CompoundTabContainer::TransferTabBetweenContainers(from_model_index,
                                                     to_model_index);
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  // Override transfer animation so that it goes well with vertical tab strip.
  CompleteAnimationAndLayout();

  const bool is_pinned = to_model_index < NumPinnedTabs();
  bool layout_dirty = false;
  if (is_pinned && !pinned_tab_container_->GetVisible()) {
    // When the browser was initialized without any pinned tabs, pinned tabs
    // could be hidden initially by the FlexLayout.
    pinned_tab_container_->SetVisible(true);
    layout_dirty = true;
  }

  // Animate tab from left to right.
  Tab* tab = GetTabAtModelIndex(to_model_index);
  tab->SetPosition({-tab->width(), 0});
  TabContainer& to_container =
      *(is_pinned ? pinned_tab_container_ : unpinned_tab_container_);
  to_container.AnimateToIdealBounds();

  if (was_pinned != is_pinned) {
    // After transferring a tab from one to another container, we should layout
    // the previous container.
    auto previous_container =
        was_pinned ? pinned_tab_container_ : unpinned_tab_container_;
    previous_container->CompleteAnimationAndLayout();
    PreferredSizeChanged();
    layout_dirty = true;
  }

  if (layout_dirty)
    Layout();
}

void BraveCompoundTabContainer::Layout() {
  if (!ShouldShowVerticalTabs()) {
    CompoundTabContainer::Layout();
    return;
  }

  // We use flex layout manager so let it do its job.
  views::View::Layout();
}

gfx::Size BraveCompoundTabContainer::CalculatePreferredSize() const {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::CalculatePreferredSize();
  }

  // We use flex layout manager so let it do its job.
  auto preferred_size = views::View::CalculatePreferredSize();

  // Check if we can expand height to fill the entire scroll area's viewport.
  for (auto* parent_view = parent(); parent_view;
       parent_view = parent_view->parent()) {
    auto* region_view =
        views::AsViewClass<VerticalTabStripRegionView>(parent_view);
    if (!region_view) {
      continue;
    }
    preferred_size.set_height(std::max(
        region_view->GetScrollViewViewportHeight(), preferred_size.height()));
    break;
  }

  return preferred_size;
}

gfx::Size BraveCompoundTabContainer::GetMinimumSize() const {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::GetMinimumSize();
  }

  // We use flex layout manager so let it do its job.
  return views::View::GetMinimumSize();
}

views::SizeBounds BraveCompoundTabContainer::GetAvailableSize(
    const views::View* child) const {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::GetAvailableSize(child);
  }

  return views::SizeBounds(views::SizeBound(width()),
                           /*height=*/views::SizeBound());
}

Tab* BraveCompoundTabContainer::AddTab(std::unique_ptr<Tab> tab,
                                       int model_index,
                                       TabPinned pinned) {
  auto* result =
      CompoundTabContainer::AddTab(std::move(tab), model_index, pinned);
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) ||
      !tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return result;
  }

  if (pinned == TabPinned::kPinned && !pinned_tab_container_->GetVisible()) {
    // When the browser was initialized without any pinned tabs, pinned tabs
    // could be hidden initially by the FlexLayout.
    pinned_tab_container_->SetVisible(true);
  }
  return result;
}

int BraveCompoundTabContainer::GetUnpinnedContainerIdealLeadingX() const {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::GetUnpinnedContainerIdealLeadingX();
  }

  return 0;
}

BrowserRootView::DropTarget* BraveCompoundTabContainer::GetDropTarget(
    gfx::Point loc_in_local_coords) {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::GetDropTarget(loc_in_local_coords);
  }

  // At this moment, upstream doesn't have implementation for this path yet.
  // TODO(1346023): Implement text drag and drop.

  if (!GetLocalBounds().Contains(loc_in_local_coords)) {
    return nullptr;
  }

  return GetTabContainerAt(loc_in_local_coords);
}

void BraveCompoundTabContainer::OnThemeChanged() {
  CompoundTabContainer::OnThemeChanged();

  if (ShouldShowVerticalTabs()) {
    pinned_tab_container_->SetBorder(views::CreateSolidSidedBorder(
        gfx::Insets().set_bottom(1),
        GetColorProvider()->GetColor(kColorBraveVerticalTabSeparator)));
  } else {
    pinned_tab_container_->SetBorder(nullptr);
  }
}

TabContainer* BraveCompoundTabContainer::GetTabContainerAt(
    gfx::Point point_in_local_coords) const {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::GetTabContainerAt(point_in_local_coords);
  }

  return point_in_local_coords.y() < pinned_tab_container_->bounds().bottom()
             ? base::to_address(pinned_tab_container_)
             : base::to_address(unpinned_tab_container_);
}

gfx::Rect BraveCompoundTabContainer::ConvertUnpinnedContainerIdealBoundsToLocal(
    gfx::Rect ideal_bounds) const {
  if (!ShouldShowVerticalTabs()) {
    return CompoundTabContainer::ConvertUnpinnedContainerIdealBoundsToLocal(
        ideal_bounds);
  }

  ideal_bounds.Offset(0, unpinned_tab_container_->y());
  return ideal_bounds;
}

bool BraveCompoundTabContainer::ShouldShowVerticalTabs() const {
  return base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) &&
         tabs::utils::ShouldShowVerticalTabs(
             tab_slot_controller_->GetBrowser());
}

BEGIN_METADATA(BraveCompoundTabContainer, CompoundTabContainer)
END_METADATA
