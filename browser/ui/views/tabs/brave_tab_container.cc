/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include "brave/browser/ui/views/tabs/brave_vertical_tab_utils.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveTabContainer::BraveTabContainer(
    TabStripController* controller,
    TabHoverCardController* hover_card_controller,
    TabDragContext* drag_context,
    TabSlotController* tab_slot_controller,
    views::View* scroll_contents_view)
    : TabContainer(controller,
                   hover_card_controller,
                   drag_context,
                   tab_slot_controller,
                   scroll_contents_view) {
  layout_helper_->set_use_vertical_tabs(tabs::ShouldShowVerticalTabs());
}

BraveTabContainer::~BraveTabContainer() = default;

gfx::Size BraveTabContainer::CalculatePreferredSize() const {
  if (tabs::ShouldShowVerticalTabs()) {
    return gfx::Size(TabStyle::GetStandardWidth(),
                     GetLayoutConstant(TAB_HEIGHT) *
                         (tabs_view_model_.view_size() + group_views_.size()));
  }

  return TabContainer::CalculatePreferredSize();
}

void BraveTabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                      bool was_active) {
  if (tabs::ShouldShowVerticalTabs())
    return;

  TabContainer::UpdateClosingModeOnRemovedTab(model_index, was_active);
}

BEGIN_METADATA(BraveTabContainer, TabContainer)
END_METADATA
