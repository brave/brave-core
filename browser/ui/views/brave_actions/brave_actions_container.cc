/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/rounded_separator.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ui/views/brave_actions/brave_rewards_action_view.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#endif

BraveActionsContainer::BraveActionsContainer(
    BrowserWindowInterface* browser_window_interface,
    Profile* profile)
    : browser_window_interface_(browser_window_interface) {}

BraveActionsContainer::~BraveActionsContainer() = default;

namespace {
constexpr int kSeparatorMargin = 3;
constexpr int kSeparatorWidth = 1;
// Total width consumed by the separator including its left/right margins.
constexpr int kSeparatorTotalWidth = kSeparatorWidth + 2 * kSeparatorMargin;
}  // namespace

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
  brave_button_separator_->SetPreferredSize(
      gfx::Size(kSeparatorWidth + kSeparatorMargin * 2,
                GetLayoutConstant(LayoutConstant::kLocationBarIconSize)));
  // separator left & right margin
  brave_button_separator_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::TLBR(0, kSeparatorMargin, 0, kSeparatorMargin)));
  // Just in case the extensions load before this function does (not likely!)
  // add children to the front in reverse order.
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  AddActionViewForRewards();
#endif
  AddActionViewForShields();
  AddChildViewAt(brave_button_separator_, 0);

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  // React to Brave Rewards preferences changes.
  show_brave_rewards_button_.Init(
      brave_rewards::prefs::kShowLocationBarButton,
      browser_window_interface_->GetProfile()->GetPrefs(),
      base::BindRepeating(
          &BraveActionsContainer::OnBraveRewardsPreferencesChanged,
          base::Unretained(this)));
#endif
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
bool BraveActionsContainer::ShouldShowBraveRewardsAction() const {
  if (!brave_rewards::IsSupportedForProfile(
          browser_window_interface_->GetProfile())) {
    return false;
  }
  const PrefService* prefs =
      browser_window_interface_->GetProfile()->GetPrefs();
  return prefs->GetBoolean(brave_rewards::prefs::kShowLocationBarButton);
}
#endif

void BraveActionsContainer::AddActionViewForShields() {
  // Do not create the shields button if the shields UI is displayed in the Page
  // Info bubble.
  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return;
  }
  shields_action_btn_ = AddChildViewAt(
      std::make_unique<BraveShieldsActionView>(browser_window_interface_), 0);
  shields_action_btn_->SetPreferredSize(GetActionSize());
  shields_action_btn_->Init();
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void BraveActionsContainer::AddActionViewForRewards() {
  auto button =
      std::make_unique<BraveRewardsActionView>(browser_window_interface_);
  rewards_action_btn_ = AddChildViewAt(std::move(button), 0);
  rewards_action_btn_->SetPreferredSize(GetActionSize());
  rewards_action_btn_->SetVisible(ShouldShowBraveRewardsAction());
  rewards_action_btn_->Update();
}
#endif

void BraveActionsContainer::Update() {
  if (shields_action_btn_) {
    shields_action_btn_->Update();
  }

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  if (rewards_action_btn_) {
    rewards_action_btn_->Update();
  }
#endif

  UpdateVisibility();
  DeprecatedLayoutImmediately();
}

void BraveActionsContainer::UpdateVisibility() {
  bool can_show = false;

  if (shields_action_btn_) {
    can_show = shields_action_btn_->GetVisible();
  }

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  if (rewards_action_btn_) {
    can_show = can_show || rewards_action_btn_->GetVisible();
  }
#endif

  // If no buttons are visible, then we want to hide this view so that the
  // separator is not displayed.
  SetVisible(!should_hide_ && can_show);
}

gfx::Size BraveActionsContainer::GetActionSize() const {
  return {34, GetLayoutConstant(LayoutConstant::kLocationBarHeight) -
                  2 * GetLayoutConstant(
                          LayoutConstant::kLocationBarElementPadding)};
}

void BraveActionsContainer::SetShouldHide(bool should_hide) {
  should_hide_ = should_hide;
  Update();
}

void BraveActionsContainer::UpdateLayoutForAvailableWidth(int available_width) {
  // Determine the natural (pref-driven) visibility for each button,
  // ignoring space constraints.
  const bool natural_shields = shields_action_btn_ != nullptr;
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  const bool natural_rewards =
      rewards_action_btn_ != nullptr && ShouldShowBraveRewardsAction();
#endif

  // Greedily assign space in priority order: Shields > Rewards.
  // The separator (kSeparatorTotalWidth) is always counted when at least one
  // button is shown, because it is always a child of this container.
  const int btn_width = GetActionSize().width();
  bool show_shields = false;
  if (natural_shields && kSeparatorTotalWidth + btn_width <= available_width) {
    show_shields = true;
  }

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  bool show_rewards = false;
  if (natural_rewards) {
    // If shields is already shown we need room for one more button; otherwise
    // we still need the separator plus one button.
    const int needed =
        kSeparatorTotalWidth + btn_width * (show_shields ? 2 : 1);
    if (needed <= available_width) {
      show_rewards = true;
    }
  }
#endif

  if (shields_action_btn_) {
    shields_action_btn_->SetVisible(show_shields);
  }
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  if (rewards_action_btn_) {
    rewards_action_btn_->SetVisible(show_rewards);
  }
#endif
  UpdateVisibility();
}

void BraveActionsContainer::ChildPreferredSizeChanged(views::View* child) {
  PreferredSizeChanged();
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
// Brave Rewards preferences change observers callback
void BraveActionsContainer::OnBraveRewardsPreferencesChanged() {
  if (rewards_action_btn_) {
    rewards_action_btn_->SetVisible(ShouldShowBraveRewardsAction());
    UpdateVisibility();
  }
}
#endif

BEGIN_METADATA(BraveActionsContainer)
END_METADATA
