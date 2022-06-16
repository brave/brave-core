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
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_resource_provider.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class AdBlockServiceTest;
class BraveAdBlockTPNetworkDelegateHelperTest;
class DomainBlockTest;
class EphemeralStorage1pDomainBlockBrowserTest;
class PerfPredictorTabHelperTest;
class PrefChangeRegistrar;
class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace adblock {
struct FilterListMetadata;
}  // namespace adblock

namespace brave_shields {

class AdBlockEngine;
class AdBlockDefaultFiltersProvider;
class AdBlockRegionalServiceManager;
class AdBlockCustomFiltersProvider;
class AdBlockRegionalCatalogProvider;
class AdBlockSubscriptionServiceManager;
class BlockDecision;

// The brave shields service in charge of ad-block checking and init.
class AdBlockService {
 public:
  class SourceProviderObserver : public AdBlockResourceProvider::Observer,
                                 public AdBlockFiltersProvider::Observer {
   public:
    SourceProviderObserver(
        base::WeakPtr<AdBlockEngine> adblock_engine,
        AdBlockFiltersProvider* source_provider,
        AdBlockResourceProvider* resource_provider,
        scoped_refptr<base::SequencedTaskRunner> task_runner,
        base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
            on_metadata_retrieved = base::DoNothing());
    SourceProviderObserver(const SourceProviderObserver&) = delete;
    SourceProviderObserver& operator=(const SourceProviderObserver&) = delete;
    ~SourceProviderObserver() override;

   private:
    // AdBlockFiltersProvider::Observer
    void OnDATLoaded(bool deserialize,
                     const DATFileDataBuffer& dat_buf) override;

    // AdBlockResourceProvider::Observer
    void OnResourcesLoaded(const std::string& resources_json) override;

    void OnEngineReplaced(
        const absl::optional<adblock::FilterListMetadata> maybe_metadata);

    bool deserialize_;
    DATFileDataBuffer dat_buf_;
    base::WeakPtr<AdBlockEngine> adblock_engine_;
    raw_ptr<AdBlockFiltersProvider> filters_provider_;    // not owned
    raw_ptr<AdBlockResourceProvider> resource_provider_;  // not owned
    base::RepeatingCallback<void(const adblock::FilterListMetadata&)>
        on_metadata_retrieved_;
    scoped_refptr<base::SequencedTaskRunner> task_runner_;

    base::WeakPtrFactory<SourceProviderObserver> weak_factory_{this};
  };

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
                          std::string* mock_data_url,
                          std::unique_ptr<BlockDecision>* block_decision);
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

  AdBlockCustomFiltersProvider* custom_filters_provider();

  void EnableTag(const std::string& tag, bool enabled);

  base::SequencedTaskRunner* GetTaskRunner();

  bool Start();

 private:
  friend class ::BraveAdBlockTPNetworkDelegateHelperTest;
  friend class ::AdBlockServiceTest;
  friend class ::DomainBlockTest;
  friend class ::EphemeralStorage1pDomainBlockBrowserTest;
  friend class ::PerfPredictorTabHelperTest;

  static std::string g_ad_block_dat_file_version_;

  AdBlockResourceProvider* resource_provider();

  void UseSourceProvidersForTest(AdBlockFiltersProvider* source_provider,
                                 AdBlockResourceProvider* resource_provider);
  void UseCustomSourceProvidersForTest(
      AdBlockFiltersProvider* source_provider,
      AdBlockResourceProvider* resource_provider);
  bool TagExistsForTest(const std::string& tag);

  raw_ptr<PrefService> local_state_;
  std::string locale_;

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::unique_ptr<brave_shields::AdBlockCustomFiltersProvider>
      custom_filters_provider_;
  std::unique_ptr<brave_shields::AdBlockDefaultFiltersProvider>
      default_filters_provider_;

  std::unique_ptr<brave_shields::AdBlockRegionalServiceManager>
      regional_service_manager_;
  std::unique_ptr<brave_shields::AdBlockEngine, base::OnTaskRunnerDeleter>
      custom_filters_service_;
  std::unique_ptr<brave_shields::AdBlockEngine, base::OnTaskRunnerDeleter>
      default_service_;
  std::unique_ptr<brave_shields::AdBlockSubscriptionServiceManager>
      subscription_service_manager_;

  std::unique_ptr<SourceProviderObserver> default_service_observer_;
  std::unique_ptr<SourceProviderObserver> custom_filters_service_observer_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AdBlockService> weak_factory_{this};
};

// Registers the local_state preferences used by Adblock
void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_
