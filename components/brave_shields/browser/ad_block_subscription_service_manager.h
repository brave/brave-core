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

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class PrefService;

namespace base {
class ListValue;
class FilePath;
}  // namespace base

class AdBlockServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

using SubscriptionIdentifier = GURL;

// The AdBlock subscription service manager, in charge of initializing and
// managing AdBlock clients corresponding to custom filter list subscriptions.
class AdBlockSubscriptionServiceManager : public ProfileManagerObserver {
 public:
  explicit AdBlockSubscriptionServiceManager(
      BraveComponent::Delegate* delegate);
  ~AdBlockSubscriptionServiceManager() override;

  // ProfileManagerObserver
  void OnProfileAdded(Profile* profile) override;

  std::vector<FilterListSubscriptionInfo> GetSubscriptions() const;
  void EnableSubscription(const SubscriptionIdentifier& id, bool enabled);
  void DeleteSubscription(const SubscriptionIdentifier& id);
  void RefreshSubscription(const SubscriptionIdentifier& id);
  void CreateSubscription(const GURL list_url);

  bool IsInitialized() const;
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

 private:
  friend class ::AdBlockServiceTest;
  bool Init();
  void StartSubscriptionServices();
  void InitializeDownloadManager(Profile* system_profile);
  void UpdateFilterListPrefs(const SubscriptionIdentifier& uuid,
                             const FilterListSubscriptionInfo& info);
  void ClearFilterListPrefs(const SubscriptionIdentifier& uuid);

  void InitializeSystemProfile();
  void OnSystemProfileCreated(Profile* profile, Profile::CreateStatus status);

  brave_component_updater::BraveComponent::Delegate* delegate_;  // NOT OWNED
  bool initialized_;
  std::map<SubscriptionIdentifier, std::unique_ptr<AdBlockSubscriptionService>>
      subscription_services_;

  std::unique_ptr<AdBlockSubscriptionDownloadManager> download_manager_;

  base::WeakPtrFactory<AdBlockSubscriptionServiceManager> weak_ptr_factory_{
      this};

  AdBlockSubscriptionServiceManager(const AdBlockSubscriptionServiceManager&) =
      delete;
  AdBlockSubscriptionServiceManager& operator=(
      const AdBlockSubscriptionServiceManager&) = delete;
};

// Creates the AdBlockSubscriptionServiceManager
std::unique_ptr<AdBlockSubscriptionServiceManager>
AdBlockSubscriptionServiceManagerFactory(BraveComponent::Delegate* delegate);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SERVICE_MANAGER_H_
