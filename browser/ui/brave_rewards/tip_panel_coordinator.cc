/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_rewards/tip_panel_coordinator.h"

#include <string>
#include <utility>

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"

namespace brave_rewards {

namespace {

void OpenRewardsPanel(Browser* browser) {
  if (auto* coordinator = RewardsPanelCoordinator::FromBrowser(browser)) {
    coordinator->OpenRewardsPanel();
  }
}

}  // namespace

TipPanelCoordinator::TipPanelCoordinator(Browser* browser,
                                         RewardsService* rewards_service)
    : BrowserUserData<TipPanelCoordinator>(*browser),
      rewards_service_(rewards_service) {
  DCHECK(rewards_service_);
}

TipPanelCoordinator::~TipPanelCoordinator() = default;

void TipPanelCoordinator::ShowPanelForPublisher(
    const std::string& publisher_id) {
  rewards_service_->GetUserType(
      base::BindOnce(&TipPanelCoordinator::GetUserTypeCallback,
                     weak_factory_.GetWeakPtr(), publisher_id));
}

void TipPanelCoordinator::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void TipPanelCoordinator::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void TipPanelCoordinator::GetUserTypeCallback(const std::string& publisher_id,
                                              mojom::UserType user_type) {
  // If the user is not "connected" (i.e. if they have not linked an external
  // wallet and they are not a "legacy" anonymous user), then open the Rewards
  // panel instead.
  if (user_type == mojom::UserType::kUnconnected) {
    OpenRewardsPanel(&GetBrowser());
    return;
  }

  rewards_service_->IsPublisherRegistered(
      publisher_id,
      base::BindOnce(&TipPanelCoordinator::IsPublisherRegisteredCallback,
                     weak_factory_.GetWeakPtr(), publisher_id));
}

void TipPanelCoordinator::IsPublisherRegisteredCallback(
    const std::string& publisher_id,
    bool is_publisher_registered) {
  // If the creator is not "registered" (and therefore has no banner information
  // to display), then open the Rewards panel instead.
  if (!is_publisher_registered) {
    OpenRewardsPanel(&GetBrowser());
    return;
  }

  OpenPanel(publisher_id);
}

void TipPanelCoordinator::OpenPanel(const std::string& publisher_id) {
  if (GetBrowser().window()->IsMinimized()) {
    GetBrowser().window()->Restore();
  }

  publisher_id_ = publisher_id;

  for (auto& observer : observers_) {
    observer.OnTipPanelRequested(publisher_id);
  }
}

BROWSER_USER_DATA_KEY_IMPL(TipPanelCoordinator);

}  // namespace brave_rewards
