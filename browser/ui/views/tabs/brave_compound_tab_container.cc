/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"

#include <memory>

#include "brave/browser/ui/views/tabs/features.h"
#include "ui/views/layout/flex_layout.h"

BraveCompoundTabContainer::BraveCompoundTabContainer(
    raw_ref<TabContainerController> controller,
    TabHoverCardController* hover_card_controller,
    TabDragContextBase* drag_context,
    TabSlotController& tab_slot_controller,
    views::View* scroll_contents_view)
    : CompoundTabContainer(controller,
                           hover_card_controller,
                           drag_context,
                           tab_slot_controller,
                           scroll_contents_view) {
  if (tabs::features::ShouldShowVerticalTabs()) {
    SetLayoutManager(std::make_unique<views::FlexLayout>())
        ->SetOrientation(views::LayoutOrientation::kVertical);
  }
}

BraveCompoundTabContainer::~BraveCompoundTabContainer() {}
