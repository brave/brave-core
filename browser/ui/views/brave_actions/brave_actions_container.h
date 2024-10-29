/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/view.h"

class BraveActionViewController;
class BraveActionsContainerTest;
class BraveRewardsActionView;
class BraveShieldsActionView;
class RewardsBrowserTest;

namespace policy {
FORWARD_DECLARE_TEST(BraveRewardsPolicyTest, RewardsIconIsHidden);
}

namespace views {
class Button;
}

// This View contains all the built-in BraveActions such as Shields and Payments
// TODO(petemill): consider splitting to separate model, like
// ToolbarActionsModel and ToolbarActionsBar
class BraveActionsContainer : public views::View {
  METADATA_HEADER(BraveActionsContainer, views::View)

 public:
  BraveActionsContainer(BrowserWindowInterface* browser_window_interface,
                        Profile* profile);
  BraveActionsContainer(const BraveActionsContainer&) = delete;

  BraveActionsContainer& operator=(const BraveActionsContainer&) = delete;

  ~BraveActionsContainer() override;

  void Init();
  void Update();
  void SetShouldHide(bool should_hide);

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

  BraveShieldsActionView* GetShieldsActionView() { return shields_action_btn_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(policy::BraveRewardsPolicyTest, RewardsIconIsHidden);
  friend class ::BraveActionsContainerTest;
  friend class ::RewardsBrowserTest;

  bool ShouldShowBraveRewardsAction() const;
  void AddActionViewForRewards();
  void AddActionViewForShields();

  void UpdateVisibility();
  gfx::Size GetActionSize() const;

  // Brave Rewards preferences change observers callback.
  void OnBraveRewardsPreferencesChanged();

  bool should_hide_ = false;

  // The Browser this LocationBarView is in.  Note that at least
  // chromeos::SimpleWebViewDialog uses a LocationBarView outside any browser
  // window, so this may be NULL.
  raw_ptr<BrowserWindowInterface> browser_window_interface_ = nullptr;

  raw_ptr<BraveShieldsActionView> shields_action_btn_ = nullptr;
  raw_ptr<BraveRewardsActionView> rewards_action_btn_ = nullptr;

  // Listen for Brave Rewards preferences changes.
  BooleanPrefMember show_brave_rewards_button_;

  base::WeakPtrFactory<BraveActionsContainer> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTIONS_CONTAINER_H_
