// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/alert/tab_alert_controller.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/favicon_size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view_class_properties.h"

BraveTab::BraveTab(tabs::TabHandle handle, TabSlotController* controller)
    : Tab(handle, controller) {
  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    InitTreeToggleButton();
  }
}

BraveTab::~BraveTab() = default;

void BraveTab::UpdateTabStyle() {
  ResetTabStyle(TabStyleViews::CreateForTab(this));
  UpdateInsets();
  InvalidateLayout();
}

const tabs::TreeTabNode* BraveTab::GetTreeTabNode() const {
  if (tree_tab_node().has_value()) {
    return controller_->GetTreeTabNode(*tree_tab_node());
  }

  return nullptr;
}

int BraveTab::GetTreeHeight() const {
  if (tree_tab_node().has_value()) {
    return controller_->GetTreeHeight(*tree_tab_node());
  }

  return 0;
}

views::BubbleBorder::Arrow BraveTab::GetAnchorPosition() const {
  if (tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface())) {
    return views::BubbleBorder::Arrow::LEFT_TOP;
  }

  return Tab::GetAnchorPosition();
}

std::u16string BraveTab::GetRenderedTooltipText(const gfx::Point& p) const {
  auto* browser = controller_->GetBrowserWindowInterface();
  if (browser &&
      brave_tabs::AreTooltipsEnabled(browser->GetProfile()->GetPrefs())) {
    return Tab::GetTooltipText(data_.title, data_.alert_state);
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

  if (tree_toggle_button_ && tree_toggle_button_->GetVisible()) {
    selectable_width -= tree_toggle_button_->width();
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
      return;
    }

    // When we show only icon for pinned vertical tab, we want to keep it
    // centered all the time.
    if ((showing_icon_ || showing_alert_indicator_) &&
        !ShouldRenderAsNormalTab()) {
      center_icon_ = true;
    }

    // Don't optimize icon visibility when floating with completely hidden mode.
    // When floating from completely hidden, icon position could be changed
    // between centered and non-centered per tab width.
    // As it's shown from hidden and vice versa, it's hard to see
    // flickering, but flickering optimization is complicated, so just skip it.
    const bool is_floating = controller()->IsVerticalTabsFloating();
    if (is_floating &&
        tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(
            controller()->GetBrowserWindowInterface())) {
      return;
    }

    // To prevent flickering during the floating animation,
    // disable centering the icon.
    if (is_floating) {
      if (!showing_alert_indicator_) {
        showing_icon_ = true;
      }
      center_icon_ = false;
      showing_close_button_ = false;
      return;
    }

    // To prevent flickering during the collaps/expand animation,
    // put icon at center always.
    if (controller()->IsVerticalTabsAnimatingButNotFinalState()) {
      if (!showing_alert_indicator_) {
        showing_icon_ = true;
      }
      center_icon_ = true;
      showing_close_button_ = false;
      return;
    }

    return;
  }

  // To prevent flickering duing the toggle animation,
  // disable centering the icon.
  if (controller()->IsVerticalTabsAnimatingButNotFinalState()) {
    if (!showing_alert_indicator_) {
      showing_icon_ = true;
    }
    center_icon_ = false;
    showing_close_button_ = false;
    return;
  }

  if (is_at_min_width) {
    center_icon_ = true;

    const bool is_active = IsActive();
    const bool can_enter_floating_mode =
        tabs::utils::IsFloatingVerticalTabsEnabled(
            controller()->GetBrowserWindowInterface());
    const bool should_show_tree_toggle_button = tree_toggle_button_ &&
                                                IsTreeNodeCollapsed() &&
                                                HasTreeTabNodeDescendants();

    // When floating mode enabled, we don't show close button as the tab strip
    // will be expanded as soon as mouse hovers onto the tab.
    showing_close_button_ =
        !showing_alert_indicator_ && !can_enter_floating_mode && is_active;
    showing_icon_ = !showing_alert_indicator_ && !showing_close_button_ &&
                    !should_show_tree_toggle_button;
  }

  if (tree_toggle_button_ && HasTreeTabNodeDescendants() &&
      showing_close_button_) {
    // We show tree toggle button instead of close button when there are
    // descendants. We need to update the icon to show the correct collapsed
    // state, here. The toggle button's visibility is updated in Layout().
    UpdateTreeToggleButtonIcon();
  }
}

bool BraveTab::HasTreeTabNodeDescendants() const {
  if (auto* node = GetTreeTabNode()) {
    return node->height() > 0;
  }

  return false;
}

void BraveTab::LayoutTreeToggleButton() {
  if (!tree_toggle_button_) {
    return;
  }

  auto* node = GetTreeTabNode();
  if (!node) {
    tree_toggle_button_->SetVisible(false);
    return;
  }

  const bool has_descendants = HasTreeTabNodeDescendants();
  if (showing_close_button_ && has_descendants) {
    // In case of tree tab node has descendants, we show tree toggle button
    // instead of close button.
    tree_toggle_button_->SetBoundsRect(close_button_->bounds());
    close_button_->SetVisible(false);
    tree_toggle_button_->SetVisible(true);
  } else if (has_descendants && node->collapsed()) {
    // In this case, we always show tree toggle button in order to indicate that
    // this tab has descendants hidden by collapsed state.
    // Here, showing_close_button_ is false and the bounds of the close button
    // is incorrect, as upstream code skips close button when
    // showing_close_button_ is false. So we need to decide toggle button
    // bounds manually. This routine is simplified version of upstream code.
    // note that setting showing_close_button_ = true won't guarantee the
    // boolean value of showing_close_button_ to be true, as we wrapped it in
    // ControllableCloseButtonState, which considers the preference,
    // active/hovered state and etc.
    const gfx::Rect contents_rect = GetContentsBounds();
    int close_button_visible_size =
        GetLayoutConstant(LayoutConstant::kTabCloseButtonSize);
    const gfx::Size close_button_actual_size =
        close_button_->GetPreferredSize();
    const int x = std::max(
        contents_rect.right() - close_button_visible_size -
            (close_button_actual_size.width() - close_button_visible_size) / 2,
        contents_rect.CenterPoint().x() -
            (close_button_actual_size.width() / 2));
    const int y = contents_rect.CenterPoint().y() -
                  (close_button_actual_size.height() / 2);
    tree_toggle_button_->SetBoundsRect(
        gfx::Rect(x, y, close_button_actual_size.width(),
                  close_button_actual_size.height()));
    tree_toggle_button_->SetVisible(true);
  } else {
    //  Otherwise, hide the button.
    tree_toggle_button_->SetVisible(false);
  }
}

bool BraveTab::IsTreeNodeCollapsed() const {
  if (auto* node = GetTreeTabNode()) {
    return node->collapsed();
  }

  return false;
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

  LayoutTreeToggleButton();
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

void BraveTab::SetData(tabs::TabData data) {
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

TabSizeInfo BraveTab::GetTabSizeInfo() const {
  if (tabs::utils::ShouldShowBraveVerticalTabs(
          controller()->GetBrowserWindowInterface())) {
    return Tab::GetTabSizeInfo();
  }

  auto size_info = Tab::GetTabSizeInfo();
  const auto mode = controller()->GetTabMinWidthMode();
  size_info.min_active_width = GetTabMinWidthForMode(
      mode, size_info.min_active_width, size_info.standard_width);
  size_info.min_inactive_width = GetTabMinWidthForMode(
      mode, size_info.min_inactive_width, size_info.standard_width);

  if (base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip)) {
    // In case horizontal scrollable tab strip is enabled, we can have wider
    // inactive tabs.
    const int min_active_width = tab_style()->GetMinimumActiveWidth(false);
    size_info.min_inactive_width =
        std::max(size_info.min_inactive_width, min_active_width);
  }

  return size_info;
}

TabNestingInfo BraveTab::GetTabNestingInfo() const {
  if (!tree_tab_node().has_value()) {
    return TabNestingInfo{};
  }

  if (!GetTreeTabNode()) {
    // The tab model in TabStripModel could be in detached state temporarily.
    return TabNestingInfo{};
  }

  return {.tree_height = GetTreeHeight(), .level = GetTreeTabNode()->level()};
}

bool BraveTab::IsInCollapsedTreeTabNode() const {
  if (!tree_tab_node().has_value()) {
    return false;
  }

  return controller_->IsInCollapsedTreeTabNode(*tree_tab_node());
}

bool BraveTab::ShouldPaintTabAccent() const {
  return controller_->ShouldPaintTabAccent(this);
}

std::optional<TabAccentColors> BraveTab::GetTabAccentColors() const {
  return controller_->GetTabAccentColors(this);
}

ui::ImageModel BraveTab::GetTabAccentIcon() const {
  return controller_->GetTabAccentIcon(this);
}

base::WeakPtr<BraveTab> BraveTab::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void BraveTab::InitTreeToggleButton() {
  constexpr int kButtonPadding = 12;
  const int icon_size = GetLayoutConstant(LayoutConstant::kTabCloseButtonSize);
  const int button_size = icon_size + kButtonPadding;
  auto tree_toggle = std::make_unique<views::ImageButton>(base::BindRepeating(
      &BraveTab::OnTreeToggleButtonPressed, base::Unretained(this)));
  tree_toggle->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  tree_toggle->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  tree_toggle->SetPreferredSize(gfx::Size(button_size, button_size));
  tree_toggle->SetFocusBehavior(views::View::FocusBehavior::ACCESSIBLE_ONLY);
  tree_toggle->SetVisible(false);
  tree_toggle_button_ = AddChildView(std::move(tree_toggle));
}

void BraveTab::OnTreeToggleButtonPressed() {
  if (!tree_tab_node().has_value()) {
    return;
  }
  const tabs::TreeTabNode* node = GetTreeTabNode();
  if (!node) {
    return;
  }
  controller_->SetTreeTabNodeCollapsed(*tree_tab_node(), !node->collapsed());
}

void BraveTab::UpdateTreeToggleButtonIcon() {
  if (!tree_toggle_button_ || !GetTreeTabNode()) {
    return;
  }

  const int icon_size = GetLayoutConstant(LayoutConstant::kTabCloseButtonSize);
  const SkColor icon_color =
      tab_style_views()->CalculateTargetColors().foreground_color;
  const bool collapsed = GetTreeTabNode()->collapsed();
  const auto& icon = collapsed ? vector_icons::kSubmenuArrowChromeRefreshIcon
                               : vector_icons::kExpandMoreIcon;
  tree_toggle_button_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(icon, icon_color, icon_size));
  tree_toggle_button_->SetImageModel(
      views::Button::STATE_HOVERED,
      ui::ImageModel::FromVectorIcon(icon, icon_color, icon_size));
  tree_toggle_button_->SetImageModel(
      views::Button::STATE_PRESSED,
      ui::ImageModel::FromVectorIcon(icon, icon_color, icon_size));
}

// static
int BraveTab::GetTabMinWidthForMode(brave_tabs::TabMinWidthMode mode,
                                    int min_width,
                                    int standard_width) {
  switch (mode) {
    case brave_tabs::TabMinWidthMode::kDefault:
      return min_width;
    case brave_tabs::TabMinWidthMode::kMinimum:
      return min_width;
    case brave_tabs::TabMinWidthMode::kMedium:
      return 76;  // We're using this value from Firefox's default min width.
    case brave_tabs::TabMinWidthMode::kLarge:
      return standard_width / 2;
    case brave_tabs::TabMinWidthMode::kFull:
      return standard_width;
  }
}

BEGIN_METADATA(BraveTab)
END_METADATA
