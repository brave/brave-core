/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_view.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/rounded_separator.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

BraveActionsContainer::BraveActionsContainer(
    BrowserWindowInterface* browser_window_interface,
    Profile* profile)
    : browser_window_interface_(browser_window_interface) {}

BraveActionsContainer::~BraveActionsContainer() = default;

void BraveActionsContainer::Init() {
  // automatic layout
  auto vertical_container_layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal);
  vertical_container_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  vertical_container_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetLayoutManager(std::move(vertical_container_layout));

  // children
  RoundedSeparator* brave_button_separator_ = new RoundedSeparator();
  // TODO(petemill): theme color
  brave_button_separator_->SetColor(SkColorSetRGB(0xb2, 0xb5, 0xb7));
  constexpr int kSeparatorMargin = 3;
  constexpr int kSeparatorWidth = 1;
  brave_button_separator_->SetPreferredSize(
      gfx::Size(kSeparatorWidth + kSeparatorMargin * 2,
                GetLayoutConstant(LOCATION_BAR_ICON_SIZE)));
  // separator left & right margin
  brave_button_separator_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::TLBR(0, kSeparatorMargin, 0, kSeparatorMargin)));
  // Just in case the extensions load before this function does (not likely!)
  // make sure separator is at index 0
  AddChildViewAt(brave_button_separator_, 0);
  AddActionViewForShields();
  AddActionViewForRewards();

  // React to Brave Rewards preferences changes.
  show_brave_rewards_button_.Init(
      brave_rewards::prefs::kShowLocationBarButton,
      browser_window_interface_->GetProfile()->GetPrefs(),
      base::BindRepeating(
          &BraveActionsContainer::OnBraveRewardsPreferencesChanged,
          base::Unretained(this)));
}

bool BraveActionsContainer::ShouldShowBraveRewardsAction() const {
  if (!brave_rewards::IsSupportedForProfile(
          browser_window_interface_->GetProfile())) {
    return false;
  }
  const PrefService* prefs =
      browser_window_interface_->GetProfile()->GetPrefs();
  return prefs->GetBoolean(brave_rewards::prefs::kShowLocationBarButton);
}

void BraveActionsContainer::AddActionViewForShields() {
  shields_action_btn_ = AddChildViewAt(
      std::make_unique<BraveShieldsActionView>(browser_window_interface_), 1);
  shields_action_btn_->SetPreferredSize(GetActionSize());
  shields_action_btn_->Init();
}

void BraveActionsContainer::AddActionViewForRewards() {
  auto button =
      std::make_unique<BraveRewardsActionView>(browser_window_interface_);
  rewards_action_btn_ = AddChildViewAt(std::move(button), 2);
  rewards_action_btn_->SetPreferredSize(GetActionSize());
  rewards_action_btn_->SetVisible(ShouldShowBraveRewardsAction());
  rewards_action_btn_->Update();
}

void BraveActionsContainer::Update() {
  if (shields_action_btn_) {
    shields_action_btn_->Update();
  }

  if (rewards_action_btn_) {
    rewards_action_btn_->Update();
  }

  UpdateVisibility();
  DeprecatedLayoutImmediately();
}

void BraveActionsContainer::UpdateVisibility() {
  bool can_show = false;

  if (shields_action_btn_) {
    can_show = shields_action_btn_->GetVisible();
  }

  if (rewards_action_btn_) {
    can_show = can_show || rewards_action_btn_->GetVisible();
  }

  // If no buttons are visible, then we want to hide this view so that the
  // separator is not displayed.
  SetVisible(!should_hide_ && can_show);
}

gfx::Size BraveActionsContainer::GetActionSize() const {
  return {34, GetLayoutConstant(LOCATION_BAR_HEIGHT) -
                  2 * GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING)};
}

void BraveActionsContainer::SetShouldHide(bool should_hide) {
  should_hide_ = should_hide;
  Update();
}

void BraveActionsContainer::ChildPreferredSizeChanged(views::View* child) {
  PreferredSizeChanged();
}

// Brave Rewards preferences change observers callback
void BraveActionsContainer::OnBraveRewardsPreferencesChanged() {
  if (rewards_action_btn_) {
    rewards_action_btn_->SetVisible(ShouldShowBraveRewardsAction());
  }
}

BEGIN_METADATA(BraveActionsContainer)
END_METADATA
