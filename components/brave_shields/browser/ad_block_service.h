/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class AdBlockServiceTest;
class BraveAdBlockTPNetworkDelegateHelperTest;
class DomainBlockTest;
class PerfPredictorTabHelperTest;
class PrefChangeRegistrar;
class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_shields {

class AdBlockEngine;
class AdBlockDefaultSourceProvider;
class AdBlockRegionalServiceManager;
class AdBlockCustomFiltersSourceProvider;
class AdBlockResourceProvider;
class AdBlockSourceProvider;
class AdBlockSubscriptionServiceManager;

// The brave shields service in charge of ad-block checking and init.
class AdBlockService {
 public:
  explicit AdBlockService(
      PrefService* local_state,
      std::string locale,
      component_updater::ComponentUpdateService* cus,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      std::unique_ptr<AdBlockSubscriptionServiceManager> manager);
  AdBlockService(const AdBlockService&) = delete;
  AdBlockService& operator=(const AdBlockService&) = delete;
  ~AdBlockService();

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
  absl::optional<base::Value> UrlCosmeticResources(const std::string& url);
  base::Value HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  AdBlockRegionalServiceManager* regional_service_manager();
  AdBlockEngine* custom_filters_service();
  AdBlockEngine* default_service();
  AdBlockSubscriptionServiceManager* subscription_service_manager();

  AdBlockCustomFiltersSourceProvider* custom_filters_source_provider();

  void EnableTag(const std::string& tag, bool enabled);

  base::SequencedTaskRunner* GetTaskRunner() { return task_runner_.get(); }

  bool Start();

 protected:
  void OnRegionalCatalogFileDataReady(const std::string& catalog_json);

 private:
  friend class ::BraveAdBlockTPNetworkDelegateHelperTest;
  friend class ::AdBlockServiceTest;
  friend class ::DomainBlockTest;
  friend class ::PerfPredictorTabHelperTest;

  static std::string g_ad_block_dat_file_version_;

  void InitCustomFilters();
  AdBlockResourceProvider* resource_provider();

  void UseSourceProvidersForTest(AdBlockSourceProvider* source_provider,
                                 AdBlockResourceProvider* resource_provider);
  void UseCustomSourceProvidersForTest(
      AdBlockSourceProvider* source_provider,
      AdBlockResourceProvider* resource_provider);
  bool TagExistsForTest(const std::string& tag);

  raw_ptr<PrefService> local_state_;
  std::string locale_;

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::unique_ptr<brave_shields::AdBlockRegionalServiceManager>
      regional_service_manager_;
  std::unique_ptr<brave_shields::AdBlockEngine> custom_filters_service_;
  std::unique_ptr<brave_shields::AdBlockEngine> default_service_;
  std::unique_ptr<brave_shields::AdBlockSubscriptionServiceManager>
      subscription_service_manager_;

  std::unique_ptr<brave_shields::AdBlockCustomFiltersSourceProvider>
      custom_filters_source_provider_;
  std::unique_ptr<brave_shields::AdBlockDefaultSourceProvider>
      default_source_provider_;

  base::WeakPtrFactory<AdBlockService> weak_factory_{this};
};

// Registers the local_state preferences used by Adblock
void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_
