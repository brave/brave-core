/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_REWARDS_TIP_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_BRAVE_REWARDS_TIP_PANEL_COORDINATOR_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "chrome/browser/ui/browser_user_data.h"
#include "ui/gfx/geometry/size.h"

namespace brave_rewards {

class RewardsService;

// Provides a browser-scoped communication channel for components that need to
// display the tip panel and components responsible for showing the tip panel.
class TipPanelCoordinator : public BrowserUserData<TipPanelCoordinator> {
 public:
  TipPanelCoordinator(Browser* browser, RewardsService* rewards_service);

  TipPanelCoordinator(const TipPanelCoordinator&) = delete;
  TipPanelCoordinator& operator=(const TipPanelCoordinator&) = delete;

  ~TipPanelCoordinator() override;

  // Displays the tip panel for the specified publisher.
  void ShowPanelForPublisher(const std::string& publisher_id);

  class Observer : public base::CheckedObserver {
   public:
    // Called when an application component requests the tip panel.
    virtual void OnTipPanelRequested(const std::string& publisher_id) {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  using Observation = base::ScopedObservation<TipPanelCoordinator, Observer>;

  // Retrieves the publisher ID associated with the most recent tip panel
  // request.
  const std::string& publisher_id() const { return publisher_id_; }

  // Gets or sets the size of the browser in which the tip panel will be
  // displayed. These dimensions can be used by the front-end to adjust layout
  // appropriately.
  const gfx::Size& browser_size() const { return browser_size_; }
  void set_browser_size(const gfx::Size& size) { browser_size_ = size; }

 private:
  friend class BrowserUserData<TipPanelCoordinator>;

  void GetUserTypeCallback(const std::string& publisher_id,
                           mojom::UserType user_type);

  void IsPublisherRegisteredCallback(const std::string& publisher_id,
                                     bool is_publisher_registered);

  void OpenPanel(const std::string& publisher_id);

  std::string publisher_id_;
  gfx::Size browser_size_;
  base::ObserverList<Observer> observers_;
  raw_ptr<RewardsService> rewards_service_ = nullptr;
  base::WeakPtrFactory<TipPanelCoordinator> weak_factory_{this};

  BROWSER_USER_DATA_KEY_DECL();
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_BRAVE_REWARDS_TIP_PANEL_COORDINATOR_H_
