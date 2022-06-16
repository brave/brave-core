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
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/sequence_checker.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_shields/browser/ad_block_engine.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"
#include "components/component_updater/timer_update_scheduler.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

class PrefService;

namespace base {
template <typename StructType>
class JSONValueConverter;
}

namespace brave_shields {
class AdBlockResourceProvider;
class AdBlockSubscriptionServiceManagerObserver;
class AdBlockSubscriptionFiltersProvider;
class BlockDecision;
}  // namespace brave_shields

class AdBlockServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

struct SubscriptionInfo {
  SubscriptionInfo();
  ~SubscriptionInfo();
  SubscriptionInfo(const SubscriptionInfo&);

  // The URL used to fetch the list, which is also used as a unique identifier
  // for a subscription service.
  GURL subscription_url;

  // These are base::Time::Min() if no download has been
  // attempted/succeeded. If a subscription has been successfully downloaded,
  // both of these are exactly equal.
  base::Time last_update_attempt;
  base::Time last_successful_update_attempt;

  // Any enabled list will be queried during network requests and page loads,
  // otherwise it will be bypassed. Disabled lists will not be automatically
  // updated.
  bool enabled;

  absl::optional<std::string> homepage;
  absl::optional<std::string> title;

  static void RegisterJSONConverter(
      base::JSONValueConverter<SubscriptionInfo>*);
};

// The AdBlock subscription service manager, in charge of initializing and
// managing AdBlock clients corresponding to custom filter list subscriptions.
class AdBlockSubscriptionServiceManager {
 public:
  explicit AdBlockSubscriptionServiceManager(
      PrefService* local_state,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      AdBlockSubscriptionDownloadManager::DownloadManagerGetter getter,
      const base::FilePath& profile_dir);
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
                          bool aggressive_blocking,
                          bool* did_match_rule,
                          bool* did_match_exception,
                          bool* did_match_important,
                          std::string* mock_data_url,
                          std::unique_ptr<BlockDecision>* block_decision);
  void EnableTag(const std::string& tag, bool enabled);
  void AddResources(const std::string& resources);

  absl::optional<base::Value> UrlCosmeticResources(const std::string& url);
  base::Value HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  AdBlockSubscriptionDownloadManager* download_manager() {
    return download_manager_.get();
  }

  void OnSubscriptionDownloadFailure(const GURL& sub_url);
  void OnSubscriptionDownloaded(const GURL& sub_url);

  void AddObserver(AdBlockSubscriptionServiceManagerObserver* observer);
  void RemoveObserver(AdBlockSubscriptionServiceManagerObserver* observer);

  void Init(AdBlockResourceProvider* resource_provider);
  bool IsInitialized();

 private:
  friend class ::AdBlockServiceTest;
  // Returns the directory used to store cached list data for the given
  // subscription.
  base::FilePath GetSubscriptionPath(const GURL& subscription_url) const;

  void OnUpdateTimer(
      component_updater::TimerUpdateScheduler::OnFinishedCallback on_finished);

  void StartDownload(const GURL& sub_url, bool from_ui);

  bool initialized_;
  void LoadSubscriptionServices();
  void UpdateSubscriptionPrefs(const GURL& sub_url,
                               const SubscriptionInfo& info);
  void ClearSubscriptionPrefs(const GURL& sub_url);
  void OnGetDownloadManager(
      AdBlockSubscriptionDownloadManager* download_manager);

  void OnListMetadata(const GURL& sub_url,
                      const adblock::FilterListMetadata& metadata);

  // static to enforce locking on `subscriptions_`
  static absl::optional<SubscriptionInfo> GetInfo(
      const base::Value::Dict& subscriptions,
      const GURL& sub_url);
  void NotifyObserversOfServiceEvent();

  void SetUpdateIntervalsForTesting(base::TimeDelta* initial_delay,
                                    base::TimeDelta* retry_interval);

  raw_ptr<PrefService> local_state_ GUARDED_BY_CONTEXT(sequence_checker_);
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  raw_ptr<AdBlockResourceProvider> resource_provider_;
  raw_ptr<brave_component_updater::BraveComponent::Delegate>
      delegate_;  // NOT OWNED
  base::WeakPtr<AdBlockSubscriptionDownloadManager> download_manager_;
  base::FilePath subscription_path_;
  base::Value::Dict subscriptions_ GUARDED_BY(subscription_services_lock_);

  std::map<GURL, std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>>
      subscription_services_ GUARDED_BY(subscription_services_lock_);
  std::map<GURL, std::unique_ptr<AdBlockSubscriptionFiltersProvider>>
      subscription_filters_providers_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::map<GURL, std::unique_ptr<AdBlockService::SourceProviderObserver>>
      subscription_source_observers_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<component_updater::TimerUpdateScheduler>
      subscription_update_timer_;

  base::ObserverList<AdBlockSubscriptionServiceManagerObserver> observers_;
  base::Lock subscription_services_lock_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AdBlockSubscriptionServiceManager> weak_ptr_factory_{
      this};

  AdBlockSubscriptionServiceManager(const AdBlockSubscriptionServiceManager&) =
      delete;
  AdBlockSubscriptionServiceManager& operator=(
      const AdBlockSubscriptionServiceManager&) = delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_MANAGER_H_
