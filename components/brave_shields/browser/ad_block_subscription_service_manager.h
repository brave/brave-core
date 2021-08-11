/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/optional.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service.h"
#include "components/component_updater/timer_update_scheduler.h"
#include "url/gurl.h"

class PrefService;

namespace brave_shields {
class AdBlockSubscriptionServiceManagerObserver;
}

class AdBlockServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

// The AdBlock subscription service manager, in charge of initializing and
// managing AdBlock clients corresponding to custom filter list subscriptions.
class AdBlockSubscriptionServiceManager {
 public:
  explicit AdBlockSubscriptionServiceManager(
      BraveComponent::Delegate* delegate,
      AdBlockSubscriptionDownloadManager::DownloadManagerGetter getter,
      const base::FilePath& user_data_dir);
  ~AdBlockSubscriptionServiceManager();

  // Returns a `file://` URL that points directly to the cached list text file
  // used for the given subscription.
  GURL GetListTextFileUrl(const GURL sub_url) const;

  std::vector<SubscriptionInfo> GetSubscriptions();
  void EnableSubscription(const GURL& sub_url, bool enabled);
  void DeleteSubscription(const GURL& sub_url);
  void RefreshSubscription(const GURL& sub_url, bool from_ui);
  void CreateSubscription(const GURL& sub_url);

  bool Start();
  void ShouldStartRequest(const GURL& url,
                          blink::mojom::ResourceType resource_type,
                          const std::string& tab_host,
                          bool* did_match_rule,
                          bool* did_match_exception,
                          bool* did_match_important,
                          std::string* mock_data_url);
  void EnableTag(const std::string& tag, bool enabled);
  void AddResources(const std::string& resources);

  base::Optional<base::Value> UrlCosmeticResources(const std::string& url);
  base::Optional<base::Value> HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  AdBlockSubscriptionDownloadManager* download_manager() {
    return download_manager_.get();
  }

  void OnListDownloadFailure(const GURL& sub_url);
  void OnListDownloaded(const GURL& sub_url);

  void AddObserver(AdBlockSubscriptionServiceManagerObserver* observer);
  void RemoveObserver(AdBlockSubscriptionServiceManagerObserver* observer);

 private:
  friend class ::AdBlockServiceTest;
  // Returns the directory used to store cached list data for the given
  // subscription.
  base::FilePath GetSubscriptionPath(const GURL& subscription_url) const;

  void OnUpdateTimer(
      const GURL& sub_url,
      bool from_ui,
      component_updater::TimerUpdateScheduler::OnFinishedCallback on_finished);

  bool Init();
  void LoadSubscriptionServices();
  void UpdateSubscriptionPrefs(const GURL& sub_url,
                               const SubscriptionInfo& info);
  void ClearSubscriptionPrefs(const GURL& sub_url);
  void OnGetDownloadManager(
      AdBlockSubscriptionDownloadManager* download_manager);

  base::Optional<SubscriptionInfo> GetInfo(
      const GURL& sub_url);
  void NotifyObserversOfServiceEvent();

  brave_component_updater::BraveComponent::Delegate* delegate_;  // NOT OWNED
  base::WeakPtr<AdBlockSubscriptionDownloadManager> download_manager_;
  base::FilePath subscription_path_;
  std::unique_ptr<base::DictionaryValue> subscriptions_;
  base::OneShotEvent ready_;

  std::map<GURL, std::unique_ptr<AdBlockSubscriptionService>>
      subscription_services_;
  std::map<GURL, std::unique_ptr<component_updater::TimerUpdateScheduler>>
      subscription_update_timers_;

  base::ObserverList<AdBlockSubscriptionServiceManagerObserver> observers_;
  base::Lock subscription_services_lock_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<AdBlockSubscriptionServiceManager> weak_ptr_factory_{
      this};

  AdBlockSubscriptionServiceManager(const AdBlockSubscriptionServiceManager&) =
      delete;
  AdBlockSubscriptionServiceManager& operator=(
      const AdBlockSubscriptionServiceManager&) = delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_MANAGER_H_
