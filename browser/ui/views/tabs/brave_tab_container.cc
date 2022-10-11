/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveTabContainer::BraveTabContainer(
    TabContainerController& controller,
    TabHoverCardController* hover_card_controller,
    TabDragContext* drag_context,
    TabSlotController& tab_slot_controller,
    views::View* scroll_contents_view)
    : TabContainerImpl(controller,
                       hover_card_controller,
                       drag_context,
                       tab_slot_controller,
                       scroll_contents_view) {
  layout_helper_->set_use_vertical_tabs(
      tabs::features::ShouldShowVerticalTabs());
}

BraveTabContainer::~BraveTabContainer() {
  // When the last tab is closed and tab strip is being destructed, the
  // animation for the last removed tab could have been scheduled but not
  // finished yet. In this case, stop the animation before checking if all
  // closed tabs were cleaned up from OnTabCloseAnimationCompleted().
  CancelAnimation();
  DCHECK(closing_tabs_.empty()) << "There are dangling closed tabs.";
}

gfx::Size BraveTabContainer::CalculatePreferredSize() const {
  if (!tabs::features::ShouldShowVerticalTabs())
    return TabContainerImpl::CalculatePreferredSize();

  const int tab_count = tabs_view_model_.view_size();
  int height = 0;
  if (bounds_animator_.IsAnimating() && tab_count) {
    // When removing a tab in the middle of tabs, the last tab's current bottom
    // could be greater than ideal bounds bottom.
    height = tabs_view_model_.view_at(tab_count - 1)->bounds().bottom();
  }

  if (!closing_tabs_.empty()) {
    // When closing trailing tabs, the last tab's current bottom could be
    // greater than ideal bounds bottom. Note that closing tabs are not in
    // tabs_view_model_ so we have to check again here.
    for (auto* tab : closing_tabs_)
      height = std::max(height, tab->bounds().bottom());
  }

  const auto slots_bounds = layout_helper_->CalculateIdealBounds({});
  height =
      std::max(height, slots_bounds.empty() ? 0 : slots_bounds.back().bottom());
  return gfx::Size(TabStyle::GetStandardWidth(), height);
}

void BraveTabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                      bool was_active) {
  if (tabs::features::ShouldShowVerticalTabs())
    return;

  TabContainerImpl::UpdateClosingModeOnRemovedTab(model_index, was_active);
}

gfx::Rect BraveTabContainer::GetTargetBoundsForClosingTab(
    Tab* tab,
    int former_model_index) const {
  if (!tabs::features::ShouldShowVerticalTabs())
    return TabContainerImpl::GetTargetBoundsForClosingTab(tab,
                                                          former_model_index);

  gfx::Rect target_bounds = tab->bounds();
  target_bounds.set_y(
      (former_model_index > 0)
          ? tabs_view_model_.ideal_bounds(former_model_index - 1).bottom()
          : 0);
  target_bounds.set_height(0);
  return target_bounds;
}

void BraveTabContainer::EnterTabClosingMode(absl::optional<int> override_width,
                                            CloseTabSource source) {
  // Don't shrink vertical tab strip's width
  if (tabs::features::ShouldShowVerticalTabs()) {
    return;
  }

  TabContainerImpl::EnterTabClosingMode(override_width, source);
}

bool BraveTabContainer::ShouldTabBeVisible(const Tab* tab) const {
  // We don't have to clip tabs out of bounds. Scroll view will handle it.
  if (tabs::features::ShouldShowVerticalTabs())
    return true;

  return TabContainerImpl::ShouldTabBeVisible(tab);
}

void BraveTabContainer::StartInsertTabAnimation(int model_index) {
  if (!tabs::features::ShouldShowVerticalTabs()) {
    TabContainerImpl::StartInsertTabAnimation(model_index);
    return;
  }

  ExitTabClosingMode();

  gfx::Rect bounds = GetTabAtModelIndex(model_index)->bounds();
  bounds.set_height(GetLayoutConstant(TAB_HEIGHT));
  bounds.set_width(TabStyle::GetStandardWidth());
  bounds.set_x(-TabStyle::GetStandardWidth());
  bounds.set_y((model_index > 0)
                   ? tabs_view_model_.ideal_bounds(model_index - 1).bottom()
                   : 0);
  GetTabAtModelIndex(model_index)->SetBoundsRect(bounds);

  // Animate in to the full width.
  StartBasicAnimation();
}

void BraveTabContainer::RemoveTab(int index, bool was_active) {
  if (tabs::features::ShouldShowVerticalTabs())
    closing_tabs_.insert(tabs_view_model_.view_at(index));

  TabContainerImpl::RemoveTab(index, was_active);
}

void BraveTabContainer::OnTabCloseAnimationCompleted(Tab* tab) {
  if (tabs::features::ShouldShowVerticalTabs())
    closing_tabs_.erase(tab);

  TabContainerImpl::OnTabCloseAnimationCompleted(tab);
}

BEGIN_METADATA(BraveTabContainer, TabContainerImpl)
END_METADATA
