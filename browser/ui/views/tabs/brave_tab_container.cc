/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>

#include "base/check_is_test.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view_utils.h"

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
                       scroll_contents_view),
      drag_context_(static_cast<TabDragContext*>(drag_context)) {
  auto* browser = tab_slot_controller_->GetBrowser();
  if (!browser) {
    CHECK_IS_TEST();
    return;
  }

  if (!tabs::features::SupportsVerticalTabs(browser)) {
    return;
  }

  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled,
      browser->profile()->GetOriginalProfile()->GetPrefs(),
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));

  UpdateLayoutOrientation();
}

BraveTabContainer::~BraveTabContainer() {
  // When the last tab is closed and tab strip is being destructed, the
  // animation for the last removed tab could have been scheduled but not
  // finished yet. In this case, stop the animation before checking if all
  // closed tabs were cleaned up from OnTabCloseAnimationCompleted().
  CancelAnimation();
  DCHECK(closing_tabs_.empty()) << "There are dangling closed tabs.";
  DCHECK(!layout_locked_)
      << "The lock returned by LockLayout() shouldn't outlive this object";
}

base::OnceClosure BraveTabContainer::LockLayout() {
  DCHECK(!layout_locked_) << "LockLayout() doesn't allow reentrance";
  layout_locked_ = true;
  return base::BindOnce(&BraveTabContainer::OnUnlockLayout,
                        base::Unretained(this));
}

gfx::Size BraveTabContainer::CalculatePreferredSize() const {
  if (layout_locked_)
    return {};

  if (!tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::CalculatePreferredSize();
  }

  const int tab_count = tabs_view_model_.view_size();
  int height = 0;
  if (bounds_animator_.IsAnimating() && tab_count &&
      !drag_context_->GetDragController()->IsActive()) {
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

  const auto slots_bounds = layout_helper_->CalculateIdealBounds(
      available_width_callback_.is_null()
          ? absl::nullopt
          : absl::optional<int>(available_width_callback_.Run()));
  height =
      std::max(height, slots_bounds.empty() ? 0 : slots_bounds.back().bottom());

  if (tab_count) {
    if (Tab* last_tab = tabs_view_model_.view_at(tab_count - 1);
        last_tab->group().has_value() &&
        !controller_->IsGroupCollapsed(*last_tab->group())) {
      height += BraveTabGroupHeader::kPaddingForGroup;
    }
  }

  return gfx::Size(TabStyle::GetStandardWidth(), height);
}

void BraveTabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                      bool was_active) {
  if (tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser()))
    return;

  TabContainerImpl::UpdateClosingModeOnRemovedTab(model_index, was_active);
}

gfx::Rect BraveTabContainer::GetTargetBoundsForClosingTab(
    Tab* tab,
    int former_model_index) const {
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser()))
    return TabContainerImpl::GetTargetBoundsForClosingTab(tab,
                                                          former_model_index);

  gfx::Rect target_bounds = tab->bounds();
  if (tab->data().pinned) {
    target_bounds.set_width(0);
  } else {
    target_bounds.set_y(
        (former_model_index > 0)
            ? tabs_view_model_.ideal_bounds(former_model_index - 1).bottom()
            : 0);
    target_bounds.set_height(0);
  }
  return target_bounds;
}

void BraveTabContainer::EnterTabClosingMode(absl::optional<int> override_width,
                                            CloseTabSource source) {
  // Don't shrink vertical tab strip's width
  if (tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return;
  }

  TabContainerImpl::EnterTabClosingMode(override_width, source);
}

bool BraveTabContainer::ShouldTabBeVisible(const Tab* tab) const {
  // We don't have to clip tabs out of bounds. Scroll view will handle it.
  if (tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser()))
    return true;

  return TabContainerImpl::ShouldTabBeVisible(tab);
}

void BraveTabContainer::StartInsertTabAnimation(int model_index) {
  if (layout_locked_)
    return;

  if (!tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::StartInsertTabAnimation(model_index);
    return;
  }

  ExitTabClosingMode();

  auto* new_tab = GetTabAtModelIndex(model_index);
  gfx::Rect bounds = new_tab->bounds();
  bounds.set_height(tabs::kVerticalTabHeight);
  const auto tab_width = new_tab->data().pinned ? tabs::kVerticalTabMinWidth
                                                : TabStyle::GetStandardWidth();
  bounds.set_width(tab_width);
  bounds.set_x(-tab_width);
  bounds.set_y((model_index > 0)
                   ? tabs_view_model_.ideal_bounds(model_index - 1).bottom()
                   : 0);
  GetTabAtModelIndex(model_index)->SetBoundsRect(bounds);

  // Animate in to the full width.
  StartBasicAnimation();
}

void BraveTabContainer::RemoveTab(int index, bool was_active) {
  if (tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser()))
    closing_tabs_.insert(tabs_view_model_.view_at(index));

  TabContainerImpl::RemoveTab(index, was_active);
}

void BraveTabContainer::OnTabCloseAnimationCompleted(Tab* tab) {
  if (tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser()))
    closing_tabs_.erase(tab);

  TabContainerImpl::OnTabCloseAnimationCompleted(tab);

  // we might have to hide this container entirely
  if (!tabs_view_model_.view_size())
    PreferredSizeChanged();
}

void BraveTabContainer::UpdateLayoutOrientation() {
  layout_helper_->set_use_vertical_tabs(tabs::features::ShouldShowVerticalTabs(
      tab_slot_controller_->GetBrowser()));
  InvalidateLayout();
}

void BraveTabContainer::OnUnlockLayout() {
  layout_locked_ = false;

  InvalidateIdealBounds();
  PreferredSizeChanged();
  CompleteAnimationAndLayout();
}

void BraveTabContainer::CompleteAnimationAndLayout() {
  if (layout_locked_)
    return;

  TabContainerImpl::CompleteAnimationAndLayout();
}

void BraveTabContainer::OnPaintBackground(gfx::Canvas* canvas) {
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::OnPaintBackground(canvas);
    return;
  }

  canvas->DrawColor(GetColorProvider()->GetColor(kColorToolbar));
}

BEGIN_METADATA(BraveTabContainer, TabContainerImpl)
END_METADATA
