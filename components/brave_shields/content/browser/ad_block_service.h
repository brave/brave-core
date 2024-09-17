// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "components/prefs/pref_registry_simple.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class AdBlockServiceTest;
class EphemeralStorage1pDomainBlockBrowserTest;
class DebounceBrowserTest;
class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace adblock {
struct BlockerResult;
struct RegexManagerDiscardPolicy;
}  // namespace adblock
namespace brave_shields {

class AdBlockEngine;
class AdBlockComponentFiltersProvider;
class AdBlockDefaultResourceProvider;
class AdBlockComponentServiceManager;
class AdBlockCustomFiltersProvider;
class AdBlockLocalhostFiltersProvider;
class AdBlockFilterListCatalogProvider;
class AdBlockSubscriptionServiceManager;
class CosmeticResourceMergeTest;
class StripProceduralFiltersTest;

// The brave shields service in charge of ad-block checking and init.
class AdBlockService {
 public:
  class SourceProviderObserver : public AdBlockResourceProvider::Observer,
                                 public AdBlockFiltersProvider::Observer {
   public:
    SourceProviderObserver(AdBlockEngine* adblock_engine,
                           AdBlockFiltersProvider* filters_provider,
                           AdBlockResourceProvider* resource_provider,
                           scoped_refptr<base::SequencedTaskRunner> task_runner,
                           bool is_filter_provider_manager = false);

    SourceProviderObserver(const SourceProviderObserver&) = delete;
    SourceProviderObserver& operator=(const SourceProviderObserver&) = delete;
    ~SourceProviderObserver() override;

   private:
    void OnFilterSetCallbackLoaded(
        base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> cb);
    void OnFilterSetCreated(std::unique_ptr<rust::Box<adblock::FilterSet>>);

    // AdBlockFiltersProvider::Observer
    void OnChanged(bool is_default_engine) override;

    // AdBlockResourceProvider::Observer
    void OnResourcesLoaded(const std::string& resources_json) override;

    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set_;
    raw_ptr<AdBlockEngine> adblock_engine_;
    raw_ptr<AdBlockFiltersProvider> filters_provider_;    // not owned
    raw_ptr<AdBlockResourceProvider> resource_provider_;  // not owned
    scoped_refptr<base::SequencedTaskRunner> task_runner_;
    bool is_filter_provider_manager_;

    base::WeakPtrFactory<SourceProviderObserver> weak_factory_{this};
  };

  explicit AdBlockService(
      PrefService* local_state,
      std::string locale,
      component_updater::ComponentUpdateService* cus,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      AdBlockSubscriptionDownloadManager::DownloadManagerGetter getter,
      const base::FilePath& profile_dir);
  AdBlockService(const AdBlockService&) = delete;
  AdBlockService& operator=(const AdBlockService&) = delete;
  ~AdBlockService();

  adblock::BlockerResult ShouldStartRequest(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host,
      bool aggressive_blocking,
      bool previously_matched_rule,
      bool previously_matched_exception,
      bool previously_matched_important);
  std::optional<std::string> GetCspDirectives(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host);
  base::Value::Dict UrlCosmeticResources(const std::string& url,
                                         bool aggressive_blocking);
  base::Value::Dict HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  AdBlockComponentServiceManager* component_service_manager();
  AdBlockSubscriptionServiceManager* subscription_service_manager();
  AdBlockCustomFiltersProvider* custom_filters_provider();

  void EnableTag(const std::string& tag, bool enabled);
  void AddUserCosmeticFilter(const std::string& filter);

  // Methods for brave://adblock-internals.
  using GetDebugInfoCallback =
      base::OnceCallback<void(base::Value::Dict, base::Value::Dict)>;
  void GetDebugInfoAsync(GetDebugInfoCallback callback);
  void DiscardRegex(uint64_t regex_id);

  void SetupDiscardPolicy(const adblock::RegexManagerDiscardPolicy& policy);

  base::SequencedTaskRunner* GetTaskRunner();

  void UseSourceProviderForTest(AdBlockFiltersProvider* source_provider);
  void UseCustomSourceProviderForTest(AdBlockFiltersProvider* source_provider);

 private:
  friend class ::AdBlockServiceTest;
  friend class ::EphemeralStorage1pDomainBlockBrowserTest;
  friend class ::DebounceBrowserTest;
  friend class brave_shields::CosmeticResourceMergeTest;
  friend class brave_shields::StripProceduralFiltersTest;

  static std::string g_ad_block_dat_file_version_;

  AdBlockResourceProvider* resource_provider();
  AdBlockComponentFiltersProvider* default_filters_provider() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return default_filters_provider_.get();
  }

  void OnGetDebugInfoFromDefaultEngine(
      GetDebugInfoCallback callback,
      base::Value::Dict default_engine_debug_info);

  void TagExistsForTest(const std::string& tag,
                        base::OnceCallback<void(bool)> cb);

  static void MergeResourcesInto(base::Value::Dict from,
                                 base::Value::Dict& into,
                                 bool force_hide);

  static void StripProceduralFilters(base::Value::Dict& resources);

  raw_ptr<PrefService> local_state_;
  std::string locale_;
  base::FilePath profile_dir_;

  AdBlockSubscriptionDownloadManager::DownloadManagerGetter
      subscription_download_manager_getter_;

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  AdBlockListP3A list_p3a_;

  std::unique_ptr<AdBlockDefaultResourceProvider> resource_provider_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockCustomFiltersProvider> custom_filters_provider_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockLocalhostFiltersProvider> localhost_filters_provider_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockComponentFiltersProvider> default_filters_provider_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockComponentFiltersProvider>
      default_exception_filters_provider_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockFilterListCatalogProvider>
      filter_list_catalog_provider_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockSubscriptionServiceManager>
      subscription_service_manager_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockComponentServiceManager> component_service_manager_
      GUARDED_BY_CONTEXT(sequence_checker_);

  std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter> default_engine_;
  std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>
      additional_filters_engine_;

  std::unique_ptr<SourceProviderObserver> default_service_observer_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<SourceProviderObserver> additional_filters_service_observer_
      GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AdBlockService> weak_factory_{this};
};

// Registers the local_state preferences used by Adblock
void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_SERVICE_H_
