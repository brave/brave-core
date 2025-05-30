/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_REWARDS_REWARDS_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_BRAVE_REWARDS_REWARDS_PANEL_COORDINATOR_H_

#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/browser_user_data.h"
#include "url/gurl.h"

namespace brave_rewards {

// Provides a browser-scoped communication channel for components that need to
// display the Rewards panel and components responsible for showing the Rewards
// panel.
class RewardsPanelCoordinator
    : public BrowserUserData<RewardsPanelCoordinator> {
 public:
  explicit RewardsPanelCoordinator(Browser* browser);

  RewardsPanelCoordinator(const RewardsPanelCoordinator&) = delete;
  RewardsPanelCoordinator& operator=(const RewardsPanelCoordinator&) = delete;

  ~RewardsPanelCoordinator() override;

  static bool IsRewardsPanelURLForTesting(const GURL& url);

  // Opens the Rewards panel with the default view.
  bool OpenRewardsPanel();

  class Observer : public base::CheckedObserver {
   public:
    // Called when an application component requests that the Rewards panel be
    // opened.
    virtual void OnRewardsPanelRequested() {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  using Observation =
      base::ScopedObservation<RewardsPanelCoordinator, Observer>;

 private:
  friend class BrowserUserData<RewardsPanelCoordinator>;

  base::ObserverList<Observer> observers_;

  BROWSER_USER_DATA_KEY_DECL();
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_BRAVE_REWARDS_REWARDS_PANEL_COORDINATOR_H_
