/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_compound_tab_container.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/ui_features.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/fill_layout.h"
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
                           scroll_contents_view),
      tab_slot_controller_(tab_slot_controller) {}

void BraveCompoundTabContainer::SetAvailableWidthCallback(
    base::RepeatingCallback<int()> available_width_callback) {
  CompoundTabContainer::SetAvailableWidthCallback(available_width_callback);
  if (!available_width_callback || tabs::features::ShouldShowVerticalTabs(
                                       tab_slot_controller_->GetBrowser())) {
    // Unlike Chromium Impl, Just pass the `available_width_callback` to
    // child containers when it's vertical tabs or we're trying to clear
    // the callback.
    pinned_tab_container_->SetAvailableWidthCallback(available_width_callback_);
    unpinned_tab_container_->SetAvailableWidthCallback(
        available_width_callback_);
  }
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

int BraveCompoundTabContainer::GetAvailableWidthForUnpinnedTabContainer(
    base::RepeatingCallback<int()> available_width_callback) {
  // At this moment, Chromium upstream has a bug which causes crash.
  // In a near future, this patch won't be needed as upstream checks if the
  // `available_width_callback` is null.
  if (!available_width_callback) {
    return parent() ? parent()->GetAvailableSize(this).width().value() : 0;
  }

  return CompoundTabContainer::GetAvailableWidthForUnpinnedTabContainer(
      available_width_callback);
}

void BraveCompoundTabContainer::TransferTabBetweenContainers(
    int from_model_index,
    int to_model_index) {
  CompoundTabContainer::TransferTabBetweenContainers(from_model_index,
                                                     to_model_index);
  if (to_model_index < NumPinnedTabs() &&
      !pinned_tab_container_->GetVisible()) {
    // When the browser was initialized without any pinned tabs, pinned tabs
    // could be hidden initially by the FlexLayout.
    pinned_tab_container_->SetVisible(true);
    Layout();
  }
}

BEGIN_METADATA(BraveCompoundTabContainer, CompoundTabContainer)
END_METADATA
