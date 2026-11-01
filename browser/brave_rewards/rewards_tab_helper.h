/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_rewards/creator_detection_script_injector.h"
#include "brave/components/brave_rewards/content/rewards_service_observer.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "components/sessions/core/session_id.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace brave_rewards {
class RewardsService;

// A tab helper responsible for detecting and storing the Rewards Creator ID
// corresponding to a given tab. Owned by BraveTabFeatures.
class RewardsTabHelper : public tabs::ContentsObservingTabFeature,
                         public RewardsServiceObserver {
 public:
  DECLARE_USER_DATA(RewardsTabHelper);

  static RewardsTabHelper* From(tabs::TabInterface* tab);

  explicit RewardsTabHelper(tabs::TabInterface& tab);
  RewardsTabHelper(const RewardsTabHelper&) = delete;
  RewardsTabHelper& operator=(const RewardsTabHelper&) = delete;
  ~RewardsTabHelper() override;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPublisherForTabUpdated(const std::string& publisher_id) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  using Observation = base::ScopedObservation<RewardsTabHelper, Observer>;

  // Returns the publisher ID associated with the web content loaded into this
  // tab. The publisher ID does not necessarily refer to a registered
  // publisher.
  std::string GetPublisherIdForTab() { return publisher_id_; }

  // Sets the publisher ID associated with the web content loaded into this
  // tab. This method can be used to override the default publisher ID as
  // determined by the current domain.
  void SetPublisherIdForTab(const std::string& publisher_id);

 private:
  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // RewardsServiceObserver:
  void OnRewardsInitialized(RewardsService* rewards_service) override;

  void MaybeSavePublisherInfo();

  void OnCreatorDetected(
      std::optional<CreatorDetectionScriptInjector::Result> result);

  SessionID tab_id_;
  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};
  raw_ptr<RewardsService> rewards_service_ = nullptr;
  base::ObserverList<Observer> observer_list_;
  std::string publisher_id_;
  std::unique_ptr<CreatorDetectionScriptInjector> creator_detection_ =
      std::make_unique<CreatorDetectionScriptInjector>();

  ui::ScopedUnownedUserData<RewardsTabHelper> scoped_unowned_user_data_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_
