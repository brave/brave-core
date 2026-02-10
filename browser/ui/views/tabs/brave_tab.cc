// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/alert/tab_alert_controller.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/gfx/favicon_size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/controls/label.h"
#include "ui/views/view_class_properties.h"

BraveTab::~BraveTab() = default;

void BraveTab::UpdateTabStyle() {
  ResetTabStyle(TabStyleViews::CreateForTab(this));
  UpdateInsets();
  InvalidateLayout();
}

std::u16string BraveTab::GetRenderedTooltipText(const gfx::Point& p) const {
  auto* browser = controller_->GetBrowserWindowInterface();
  if (browser &&
      brave_tabs::AreTooltipsEnabled(browser->GetProfile()->GetPrefs())) {
    return Tab::GetTooltipText(
        data_.title,
        tabs::TabAlertController::GetAlertStateToShow(data_.alert_state));
  }
  return TabSlotView::GetTooltipText();
}

int BraveTab::GetWidthOfLargestSelectableRegion() const {
  // Assume the entire region except the area that alert indicator/close buttons
  // occupied is available for click-to-select.
  // If neither are visible, the entire tab region is available.
  int selectable_width = width();
  if (alert_indicator_button_->GetVisible()) {
    selectable_width -= alert_indicator_button_->width();
  }

  if (close_button_->GetVisible()) {
    selectable_width -= close_button_->width();
  }

  return std::max(0, selectable_width);
}

void BraveTab::ActiveStateChanged() {
  Tab::ActiveStateChanged();

  // This should be called whenever the active state changes
  // see comment on UpdateEnabledForMuteToggle();
  // https://github.com/brave/brave-browser/issues/23476/
  alert_indicator_button_->UpdateEnabledForMuteToggle();
}

std::optional<SkColor> BraveTab::GetGroupColor() const {
  // Hide tab border with group color as it doesn't go well with vertical tabs.
  if (tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface())) {
    return {};
  }

  if (!tabs::HorizontalTabsUpdateEnabled()) {
    return Tab::GetGroupColor();
  }

  // Unlike upstream, tabs that are within a group are not given a border color.
  return {};
}

void BraveTab::UpdateIconVisibility() {
  Tab::UpdateIconVisibility();
  if (!tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface())) {
    return;
  }

  const auto is_at_min_width = IsAtMinWidthForVerticalTabStrip();

  if (data().pinned) {
    if (is_at_min_width) {
      // When pinned vertical tab is at min width, we show only icon.
      center_icon_ = true;
      showing_icon_ = !showing_alert_indicator_;
      showing_close_button_ = false;
    }

    // When we show only icon for pinned vertical tab, we want to keep it
    // centered all the time.
    if ((showing_icon_ || showing_alert_indicator_) &&
        !ShouldRenderAsNormalTab()) {
      center_icon_ = true;
    }
    return;
  }

  if (is_at_min_width) {
    center_icon_ = true;

    const bool is_active = IsActive();
    const bool can_enter_floating_mode =
        tabs::utils::IsFloatingVerticalTabsEnabled(
            controller()->GetBrowserWindowInterface());
    // When floating mode enabled, we don't show close button as the tab strip
    // will be expanded as soon as mouse hovers onto the tab.
    showing_close_button_ =
        !showing_alert_indicator_ && !can_enter_floating_mode && is_active;
    showing_icon_ = !showing_alert_indicator_ && !showing_close_button_;
  }
}

void BraveTab::Layout(PassKey) {
  // Try update insets - this might not be the best place to update insets.
  // This is too frequent. Storage partition config was set to a contents
  // explicitly, we should migrate these to the the notification.
  if (GetInsets().left() != tab_style_views()->GetContentsInsets().left()) {
    UpdateInsets();
  }

  LayoutSuperclass<Tab>(this);

  if (IsAtMinWidthForVerticalTabStrip()) {
    if (showing_close_button_) {
      close_button_->SetX(GetLocalBounds().CenterPoint().x() -
                          (close_button_->width() / 2));

      // In order to reset ink drop bounds based on new padding.
      auto* ink_drop = views::InkDrop::Get(close_button_)->GetInkDrop();
      DCHECK(ink_drop);
      ink_drop->HostSizeChanged(close_button_->size());
    }
  }
}

void BraveTab::MaybeAdjustLeftForPinnedTab(gfx::Rect* bounds,
                                           int visual_width) const {
  if (!tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface())) {
    Tab::MaybeAdjustLeftForPinnedTab(bounds, visual_width);
    return;
  }

  // We keep favicon on fixed position so that it won't move left and right
  // during animation.
  bounds->set_x((tabs::kVerticalTabMinWidth - gfx::kFaviconSize) / 2);
  if (ShouldPaintTabAccent() && ShouldShowLargeAccentIcon()) {
    bounds->set_x(bounds->x() + kTabAccentIconAreaWidth);
  }
}

bool BraveTab::ShouldShowLargeAccentIcon() const {
  if (!ShouldPaintTabAccent()) {
    return false;
  }

  if (data().pinned) {
    return false;
  }

  if (tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface())) {
    return width() >= tabs::kVerticalTabMinWidth + kTabAccentIconAreaWidth;
  }

  if (IsActive()) {
    return width() >= tab_style()->GetMinimumActiveWidth(split().has_value()) +
                          kTabAccentIconAreaWidth;
  }

  return width() >= tab_style()->GetPinnedWidth(split().has_value()) +
                        kTabAccentIconAreaWidth;
}

bool BraveTab::ShouldRenderAsNormalTab() const {
  if (IsAtMinWidthForVerticalTabStrip()) {
    // Returns false to hide title
    return false;
  }

  if (tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface()) &&
      data().pinned && !controller_->IsVerticalTabsFloating()) {
    // In cased of pinned vertical tabs, we never render as normal tab, i.e.
    // always show only icon.
    return false;
  }

  return Tab::ShouldRenderAsNormalTab();
}

bool BraveTab::IsAtMinWidthForVerticalTabStrip() const {
  return tabs::utils::ShouldShowBraveVerticalTabs(
             controller()->GetBrowserWindowInterface()) &&
         width() <= tabs::kVerticalTabMinWidth;
}

void BraveTab::SetData(TabRendererData data) {
  const bool data_changed = data != data_;
  Tab::SetData(std::move(data));

  // Our vertical tab uses CompoundTabContainer.
  // When tab is moved from the group by pinning, it's moved to
  // pinned TabContainerImpl before its tab group id is cleared.
  // And it causes runtime crash as using this tab from pinned TabContainerImpl
  // has assumption that it's not included in any group.
  // So, clear in-advance when tab enters to pinned TabContainerImpl.
  if (data_changed &&
      tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface()) &&
      data_.pinned) {
    SetGroup(std::nullopt);
  }
}

bool BraveTab::IsActive() const {
  // When SideBySide is enabled, chromium gives true if tab is in split tab even
  // it's not active. We want to give true only for current active tab.
  return controller_->IsActiveTab(this);
}

bool BraveTab::ShouldPaintTabAccent() const {
  return controller_->ShouldPaintTabAccent(this);
}

std::optional<SkColor> BraveTab::GetTabAccentColor() const {
  return controller_->GetTabAccentColor(this);
}

ui::ImageModel BraveTab::GetTabAccentIcon() const {
  return controller_->GetTabAccentIcon(this);
}

BEGIN_METADATA(BraveTab)
END_METADATA
