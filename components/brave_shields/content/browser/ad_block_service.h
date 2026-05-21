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
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "base/values.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_download_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "components/prefs/pref_registry_simple.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "third_party/rust/cxx/v1/cxx.h"
#include "url/gurl.h"

class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace adblock {
struct RegexManagerDiscardPolicy;
}  // namespace adblock
namespace brave_shields {

class AdBlockComponentFiltersProvider;
class AdBlockDefaultResourceProvider;
class AdBlockComponentServiceManager;
class AdBlockCustomFiltersProvider;
class AdBlockCustomResourceProvider;
class AdBlockLocalhostFiltersProvider;
class AdBlockFilterListCatalogProvider;
class AdBlockSubscriptionServiceManager;

// The brave shields service in charge of ad-block checking and init.
class AdBlockService {
 public:
  enum class FilterListLoadResult {
    kLoaded,
    kFailed,
    kResourcesOnly,
  };

  class Observer : public base::CheckedObserver {
   public:
    Observer() = default;
    ~Observer() override = default;
    virtual void OnFilterListLoaded(bool is_default_engine,
                                    FilterListLoadResult result) {}
    virtual void OnDATLoaded(bool is_default_engine, bool success) {}
  };

  class SourceProviderObserver : public AdBlockResourceProvider::Observer,
                                 public AdBlockFiltersProvider::Observer {
   public:
    // Callback to handle loading resources into the engine.
    // If filter_set is non-null, calls Load; otherwise calls UseResources.
    using OnResourcesLoadedCallback = base::RepeatingCallback<void(
        bool,
        std::optional<DATFileDataBuffer>,
        std::unique_ptr<rust::Box<adblock::FilterSet>>,
        AdblockResourceStorageBox)>;

    SourceProviderObserver(
        OnResourcesLoadedCallback on_resources_loaded,
        AdBlockResourceProvider* resource_provider,
        AdBlockFiltersProviderManager* filters_provider_manager,
        bool engine_is_default,
        scoped_refptr<base::SequencedTaskRunner> task_runner);

    SourceProviderObserver(const SourceProviderObserver&) = delete;
    SourceProviderObserver& operator=(const SourceProviderObserver&) = delete;
    ~SourceProviderObserver() override;

    // AdBlockFiltersProvider::Observer
    void OnChanged(bool is_default_engine) override;

    void OnDATFileRead(DATFileDataBuffer dat);

   private:
    void LoadResources(
        std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set);
    void OnFilterSetLoaded(
        base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> cb);
    void OnFilterSetCreated(
        std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set);

    // AdBlockResourceProvider::Observer
    void OnResourcesLoaded(AdblockResourceStorageBox storage) override;

    void OnAllLoaded(std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
                     AdblockResourceStorageBox storage);

    OnResourcesLoadedCallback on_resources_loaded_;
    const bool engine_is_default_;

    scoped_refptr<base::SequencedTaskRunner> task_runner_;

    raw_ptr<AdBlockResourceProvider> resource_provider_ = nullptr;  // not owned
    raw_ptr<AdBlockResourceProvider> custom_resource_provider_ =
        nullptr;  // not owned
    raw_ptr<AdBlockFiltersProviderManager> filters_provider_manager_ =
        nullptr;  // not owned

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

  AdBlockComponentServiceManager* component_service_manager();
  AdBlockSubscriptionServiceManager* subscription_service_manager();
  AdBlockDefaultResourceProvider* default_resource_provider();
  AdBlockCustomFiltersProvider* custom_filters_provider();
  AdBlockCustomResourceProvider* custom_resource_provider();

  // Call a callback on the task runner with the engine wrapper
  void AsyncCall(base::OnceCallback<void(AdBlockEngineWrapper* wrapper)> task) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    engine_wrapper_.PostTaskWithThisObject(std::move(task));
  }

  // Call a callback on the task runner with the engine wrapper and post the
  // result back to the calling sequence (current default task runner).
  template <typename T>
  void AsyncCallAndReplyWithResult(
      base::OnceCallback<T(AdBlockEngineWrapper* wrapper)> task,
      base::OnceCallback<void(T)> reply) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    engine_wrapper_.PostTaskWithThisObject(base::BindOnce(
        [](base::OnceCallback<T(AdBlockEngineWrapper * wrapper)> task,
           base::OnceCallback<void(T)> reply,
           scoped_refptr<base::SequencedTaskRunner> task_runner,
           AdBlockEngineWrapper* wrapper) {
          auto result = std::move(task).Run(wrapper);
          task_runner->PostTask(
              FROM_HERE, base::BindOnce(std::move(reply), std::move(result)));
        },
        std::move(task), std::move(reply),
        base::SequencedTaskRunner::GetCurrentDefault()));
  }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void EnableTag(const std::string& tag, bool enabled);
  void AddUserCosmeticFilter(const std::string& filter);
  void ResetCosmeticFilter(std::string_view host);
  bool AreAnyBlockedElementsPresent(std::string_view host);

  // Methods for brave://adblock-internals.
  using GetDebugInfoCallback =
      base::OnceCallback<void(std::pair<base::DictValue, base::DictValue>)>;
  void GetDebugInfoAsync(GetDebugInfoCallback callback);
  void DiscardRegex(uint64_t regex_id);

  void SetupDiscardPolicy(const adblock::RegexManagerDiscardPolicy& policy);

  // Test accessors
  AdBlockFiltersProviderManager* GetFiltersProviderManagerForTesting();
  AdBlockDefaultResourceProvider* GetDefaultResourceProviderForTesting();
  base::SequencedTaskRunner* GetTaskRunnerForTesting();
  AdBlockDATCacheManager* GetDATCacheManagerForTesting();
  bool IsDATLoadedForTesting(bool is_default_engine) const;
  bool IsFilterListLoadedForTesting(bool is_default_engine) const;

 private:
  static std::string g_ad_block_dat_file_version_;

  void OnResourcesLoaded(
      bool is_default_engine,
      std::optional<DATFileDataBuffer> dat,
      std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
      AdblockResourceStorageBox storage);

  void OnDATLoaded(bool is_default_engine, bool success);
  void OnEngineLoaded(
      bool is_default_engine,
      std::pair<FilterListLoadResult, std::optional<DATFileDataBuffer>> result);
  void OnReadCachedDATFiles(std::optional<DATFileDataBuffer> default_dat,
                            std::optional<DATFileDataBuffer> additional_dat);

  AdBlockComponentFiltersProvider* default_filters_provider() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return default_filters_provider_.get();
  }

  AdBlockFiltersProviderManager* filters_provider_manager() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return filters_provider_manager_.get();
  }

  raw_ptr<PrefService> local_state_;
  std::string locale_;
  base::FilePath profile_dir_;

  AdBlockSubscriptionDownloadManager::DownloadManagerGetter
      subscription_download_manager_getter_;

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  AdBlockListP3A list_p3a_;

  // The AdBlockEngineWrapper should be deleted last to ensure that any code
  // that posts to the task runner will run before the deletion.

  // Lives on `task_runner_`; constructed and destroyed there via SequenceBound.
  base::SequenceBound<AdBlockEngineWrapper> engine_wrapper_
      GUARDED_BY_CONTEXT(sequence_checker_);

  std::unique_ptr<AdBlockFiltersProviderManager> filters_provider_manager_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockDATCacheManager> dat_cache_manager_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<AdBlockResourceProvider> resource_provider_
      GUARDED_BY_CONTEXT(sequence_checker_);
  raw_ptr<AdBlockDefaultResourceProvider> default_resource_provider_
      GUARDED_BY_CONTEXT(sequence_checker_) = nullptr;
  raw_ptr<AdBlockCustomResourceProvider> custom_resource_provider_
      GUARDED_BY_CONTEXT(sequence_checker_) = nullptr;
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

  std::unique_ptr<SourceProviderObserver> default_service_observer_
      GUARDED_BY_CONTEXT(sequence_checker_);
  std::unique_ptr<SourceProviderObserver> additional_filters_service_observer_
      GUARDED_BY_CONTEXT(sequence_checker_);

  base::ObserverList<Observer> observers_;

  bool default_dat_loaded_for_testing_ = false;
  bool additional_dat_loaded_for_testing_ = false;
  bool default_filter_list_loaded_for_testing_ = false;
  bool additional_filter_list_loaded_for_testing_ = false;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AdBlockService> weak_factory_{this};
};

// Registers the local_state preferences used by Adblock
void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry);

// Registers local_state Adblock preferences needed for migration
void RegisterPrefsForAdBlockServiceForMigration(PrefRegistrySimple* registry);

// Migrates or clears obsolete local_state preferences used by Adblock
void MigrateObsoletePrefsForAdBlockService(PrefService* local_state);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_SERVICE_H_
