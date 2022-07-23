/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser_list_observer.h"
#endif

class Browser;

namespace brave_rewards {

class RewardsService;

// A tab helper responsible for sending user-activity events to the Rewards
// engine in order to support the Auto Contribute feature, and for storing the
// publisher ID corresponding to a given tab.
class RewardsTabHelper : public content::WebContentsUserData<RewardsTabHelper>,
                         public content::WebContentsObserver,
#if !BUILDFLAG(IS_ANDROID)
                         public BrowserListObserver,
#endif
                         public RewardsServiceObserver {
 public:
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
  // tab. The publisher ID does not necessarily refer to a registered publisher.
  std::string GetPublisherIdForTab() { return publisher_id_; }

  // Sets the publisher ID associated with the web content loaded into this tab.
  // This method can be used to override the default publisher ID as determined
  // by the current domain.
  void SetPublisherIdForTab(const std::string& publisher_id);

 private:
  friend class content::WebContentsUserData<RewardsTabHelper>;

  explicit RewardsTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const blink::mojom::ResourceLoadInfo& resource_load_info) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

#if !BUILDFLAG(IS_ANDROID)
  // BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  bool BrowserHasWebContents(Browser* browser);
#endif

  // RewardsServiceObserver:
  void OnRewardsInitialized(RewardsService* rewards_service) override;

  void MaybeSavePublisherInfo();

  SessionID tab_id_;
  raw_ptr<RewardsService> rewards_service_ = nullptr;  // NOT OWNED
  base::ObserverList<Observer> observer_list_;
  std::string publisher_id_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_
