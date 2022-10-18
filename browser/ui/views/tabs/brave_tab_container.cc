/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveTabContainer::BraveTabContainer(
    TabContainerController& controller,
    TabHoverCardController* hover_card_controller,
    TabDragContextBase* drag_context,
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

BraveTabContainer::~BraveTabContainer() = default;

gfx::Size BraveTabContainer::CalculatePreferredSize() const {
  if (tabs::features::ShouldShowVerticalTabs()) {
    return gfx::Size(TabStyle::GetStandardWidth(),
                     GetLayoutConstant(TAB_HEIGHT) *
                         (tabs_view_model_.view_size() + group_views_.size()));
  }

  return TabContainerImpl::CalculatePreferredSize();
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
  return target_bounds;
}

BEGIN_METADATA(BraveTabContainer, TabContainerImpl)
END_METADATA
