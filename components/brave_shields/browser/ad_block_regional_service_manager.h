/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "base/values.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_shields/browser/ad_block_engine.h"
#include "brave/components/brave_shields/browser/ad_block_regional_catalog_provider.h"
#include "brave/components/brave_shields/browser/ad_block_regional_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class AdBlockServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

class AdBlockRegionalService;

// The AdBlock regional service manager, in charge of initializing and
// managing regional AdBlock clients.
class AdBlockRegionalServiceManager
    : public AdBlockRegionalCatalogProvider::Observer {
 public:
  explicit AdBlockRegionalServiceManager(
      PrefService* local_state,
      std::string locale,
      component_updater::ComponentUpdateService* cus,
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  AdBlockRegionalServiceManager(const AdBlockRegionalServiceManager&) = delete;
  AdBlockRegionalServiceManager& operator=(
      const AdBlockRegionalServiceManager&) = delete;
  ~AdBlockRegionalServiceManager() override;

  base::Value::List GetRegionalLists();

  void SetRegionalCatalog(std::vector<adblock::FilterList> catalog);
  const std::vector<adblock::FilterList>& GetRegionalCatalog();

  bool Start();
  void ShouldStartRequest(const GURL& url,
                          blink::mojom::ResourceType resource_type,
                          const std::string& tab_host,
                          bool aggressive_blocking,
                          bool* did_match_rule,
                          bool* did_match_exception,
                          bool* did_match_important,
                          std::string* mock_data_url);
  absl::optional<std::string> GetCspDirectives(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host);
  void EnableTag(const std::string& tag, bool enabled);
  void AddResources(const std::string& resources);
  bool IsFilterListAvailable(const std::string& uuid) const;
  bool IsFilterListEnabled(const std::string& uuid) const;
  void EnableFilterList(const std::string& uuid, bool enabled);

  absl::optional<base::Value> UrlCosmeticResources(const std::string& url);
  base::Value::List HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  void Init(AdBlockResourceProvider* resource_provider,
            AdBlockRegionalCatalogProvider* catalog_provider);

  // AdBlockRegionalCatalogProvider::Observer
  void OnRegionalCatalogLoaded(const std::string& catalog_json) override;

 private:
  friend class ::AdBlockServiceTest;
  void StartRegionalServices();
  void UpdateFilterListPrefs(const std::string& uuid, bool enabled);

  raw_ptr<PrefService> local_state_;
  std::string locale_;
  bool initialized_;
  base::Lock regional_services_lock_;
  std::map<std::string,
           std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>>
      regional_services_ GUARDED_BY(regional_services_lock_);
  std::map<std::string, std::unique_ptr<AdBlockRegionalFiltersProvider>>
      regional_filters_providers_;
  std::map<std::string, std::unique_ptr<AdBlockService::SourceProviderObserver>>
      regional_source_observers_;

  std::vector<adblock::FilterList> regional_catalog_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;
  raw_ptr<AdBlockResourceProvider> resource_provider_;
  raw_ptr<AdBlockRegionalCatalogProvider> catalog_provider_;

  base::WeakPtrFactory<AdBlockRegionalServiceManager> weak_factory_{this};
};

// Creates the AdBlockRegionalServiceManager
std::unique_ptr<AdBlockRegionalServiceManager>
AdBlockRegionalServiceManagerFactory(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    scoped_refptr<base::SequencedTaskRunner> task_runner);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_
