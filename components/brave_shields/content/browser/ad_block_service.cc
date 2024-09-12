// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_service.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/content/browser/ad_block_localhost_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

namespace brave_shields {

AdBlockService::SourceProviderObserver::SourceProviderObserver(
    AdBlockEngine* adblock_engine,
    AdBlockFiltersProvider* filters_provider,
    AdBlockResourceProvider* resource_provider,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    bool is_filter_provider_manager)
    : adblock_engine_(adblock_engine),
      filters_provider_(filters_provider),
      resource_provider_(resource_provider),
      task_runner_(task_runner),
      is_filter_provider_manager_(is_filter_provider_manager) {
  filters_provider_->AddObserver(this);
  OnChanged(adblock_engine->IsDefaultEngine());
}

AdBlockService::SourceProviderObserver::~SourceProviderObserver() {
  filters_provider_->RemoveObserver(this);
  resource_provider_->RemoveObserver(this);
}

void AdBlockService::SourceProviderObserver::OnChanged(bool is_default_engine) {
  if (adblock_engine_->IsDefaultEngine() != is_default_engine) {
    // Skip updates of another engine.
    return;
  }
  auto on_loaded_cb = base::BindOnce(
      &AdBlockService::SourceProviderObserver::OnFilterSetCallbackLoaded,
      weak_factory_.GetWeakPtr());
  if (is_filter_provider_manager_) {
    static_cast<AdBlockFiltersProviderManager*>(filters_provider_.get())
        ->LoadFilterSetForEngine(is_default_engine, std::move(on_loaded_cb));
  } else {
    filters_provider_->LoadFilterSet(std::move(on_loaded_cb));
  }
}

void AdBlockService::SourceProviderObserver::OnFilterSetCallbackLoaded(
    base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> cb) {
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> cb) {
            auto filter_set = std::make_unique<rust::Box<adblock::FilterSet>>(
                adblock::new_filter_set());
            std::move(cb).Run(filter_set.get());
            return filter_set;
          },
          std::move(cb)),
      base::BindOnce(
          &AdBlockService::SourceProviderObserver::OnFilterSetCreated,
          weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::OnFilterSetCreated(
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set) {
  filter_set_ = std::move(filter_set);
  // multiple AddObserver calls are ignored
  resource_provider_->AddObserver(this);
  resource_provider_->LoadResources(base::BindOnce(
      &SourceProviderObserver::OnResourcesLoaded, weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::OnResourcesLoaded(
    const std::string& resources_json) {
  if (!filter_set_) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AdBlockEngine::UseResources,
                       adblock_engine_->AsWeakPtr(), resources_json));
  } else {
    auto engine_load_callback = base::BindOnce(
        [](base::WeakPtr<AdBlockEngine> engine,
           std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
           const std::string& resources_json) {
          if (engine) {
            engine->Load(std::move(*filter_set.get()), resources_json);
          }
        },
        adblock_engine_->AsWeakPtr(), std::move(filter_set_), resources_json);
    task_runner_->PostTask(FROM_HERE, std::move(engine_load_callback));
  }
}

adblock::BlockerResult AdBlockService::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool previously_matched_rule,
    bool previously_matched_exception,
    bool previously_matched_important) {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());

  adblock::BlockerResult fp_result = default_engine_->ShouldStartRequest(
      url, resource_type, tab_host, previously_matched_rule,
      previously_matched_exception, previously_matched_important);
  if (aggressive_blocking ||
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) ||
      !SameDomainOrHost(
          url, url::Origin::CreateFromNormalizedTuple("https", tab_host, 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    // removeparam results from the default engine are ignored in default
    // blocking mode
    if (!aggressive_blocking) {
      fp_result.rewritten_url.has_value = false;
    }
    if (fp_result.important) {
      return fp_result;
    }
  } else {
    // if there's an exception from the default engine, it still needs to be
    // considered by the additional engine
    fp_result = {.has_exception = fp_result.has_exception};
  }

  GURL request_url = fp_result.rewritten_url.has_value
                         ? GURL(std::string(fp_result.rewritten_url.value))
                         : url;
  auto result = additional_filters_engine_->ShouldStartRequest(
      request_url, resource_type, tab_host,
      previously_matched_rule | fp_result.matched,
      previously_matched_exception | fp_result.has_exception,
      previously_matched_important | fp_result.important);

  result.matched |= fp_result.matched;
  result.has_exception |= fp_result.has_exception;
  result.important |= fp_result.important;
  if (!result.redirect.has_value && fp_result.redirect.has_value) {
    result.redirect = fp_result.redirect;
  }
  if (!result.rewritten_url.has_value && fp_result.rewritten_url.has_value) {
    result.rewritten_url = fp_result.rewritten_url;
  }
  return result;
}

std::optional<std::string> AdBlockService::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());
  auto csp_directives =
      default_engine_->GetCspDirectives(url, resource_type, tab_host);

  const auto additional_csp = additional_filters_engine_->GetCspDirectives(
      url, resource_type, tab_host);
  MergeCspDirectiveInto(additional_csp, &csp_directives);

  return csp_directives;
}

base::Value::Dict AdBlockService::UrlCosmeticResources(
    const std::string& url,
    bool aggressive_blocking) {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());
  base::Value::Dict resources = default_engine_->UrlCosmeticResources(url);

  if (!aggressive_blocking) {
    // `:has` procedural selectors from the default engine should not be hidden
    // in standard blocking mode.
    base::Value::List* default_hide_selectors =
        resources.FindList("hide_selectors");
    if (default_hide_selectors) {
      base::Value::List::iterator it = default_hide_selectors->begin();
      while (it < default_hide_selectors->end()) {
        DCHECK(it->is_string());
        if (it->GetString().find(":has(") != std::string::npos) {
          it = default_hide_selectors->erase(it);
        } else {
          it++;
        }
      }
    }
  }

  base::Value::Dict additional_resources =
      additional_filters_engine_->UrlCosmeticResources(url);

  MergeResourcesInto(std::move(additional_resources), resources,
                     /*force_hide=*/true);

  return resources;
}

// The return value here is formatted differently from the rest of the adblock
// service instances. We need to distinguish between selectors returned from
// the default engine and those returned by other engines, but still comply
// with the virtual method signature.
// This can be improved once interfaces are decoupled in
// https://github.com/brave/brave-core/pull/10994.
// For now, this returns a dict with two properties:
//  - "hide_selectors" - wraps the result from the default engine
//  - "force_hide_selectors" - wraps appended results from all other engines
base::Value::Dict AdBlockService::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());
  base::Value::List hide_selectors =
      default_engine_->HiddenClassIdSelectors(classes, ids, exceptions);

  base::Value::List additional_selectors =
      additional_filters_engine_->HiddenClassIdSelectors(classes, ids,
                                                         exceptions);

  base::Value::List force_hide_selectors = std::move(additional_selectors);

  base::Value::Dict result;
  result.Set("hide_selectors", std::move(hide_selectors));
  result.Set("force_hide_selectors", std::move(force_hide_selectors));
  return result;
}

AdBlockComponentServiceManager* AdBlockService::component_service_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return component_service_manager_.get();
}

AdBlockCustomFiltersProvider* AdBlockService::custom_filters_provider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return custom_filters_provider_.get();
}

AdBlockSubscriptionServiceManager*
AdBlockService::subscription_service_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return subscription_service_manager_.get();
}

AdBlockService::AdBlockService(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    AdBlockSubscriptionDownloadManager::DownloadManagerGetter
        subscription_download_manager_getter,
    const base::FilePath& profile_dir)
    : local_state_(local_state),
      locale_(locale),
      profile_dir_(profile_dir),
      subscription_download_manager_getter_(
          std::move(subscription_download_manager_getter)),
      component_update_service_(cus),
      task_runner_(task_runner),
      list_p3a_(local_state),
      default_engine_(std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
          new AdBlockEngine(true /* is_default */),
          base::OnTaskRunnerDeleter(GetTaskRunner()))),
      additional_filters_engine_(
          std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
              new AdBlockEngine(false /* is_default */),
              base::OnTaskRunnerDeleter(GetTaskRunner()))) {
  // Initializes adblock-rust's domain resolution implementation
  adblock::set_domain_resolver();

  if (base::FeatureList::IsEnabled(
          features::kAdblockOverrideRegexDiscardPolicy)) {
    adblock::RegexManagerDiscardPolicy policy;
    policy.cleanup_interval_secs =
        features::kAdblockOverrideRegexDiscardPolicyCleanupIntervalSec.Get();
    policy.discard_unused_secs =
        features::kAdblockOverrideRegexDiscardPolicyDiscardUnusedSec.Get();
    SetupDiscardPolicy(policy);
  }

  resource_provider_ = std::make_unique<AdBlockDefaultResourceProvider>(
      component_update_service_);
  filter_list_catalog_provider_ =
      std::make_unique<AdBlockFilterListCatalogProvider>(
          component_update_service_);

  component_service_manager_ = std::make_unique<AdBlockComponentServiceManager>(
      local_state_, locale_, component_update_service_,
      filter_list_catalog_provider_.get(), &list_p3a_);
  subscription_service_manager_ =
      std::make_unique<AdBlockSubscriptionServiceManager>(
          local_state_, std::move(subscription_download_manager_getter_),
          profile_dir_, &list_p3a_);
  custom_filters_provider_ =
      std::make_unique<AdBlockCustomFiltersProvider>(local_state_);

  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveLocalhostAccessPermission)) {
    localhost_filters_provider_ =
        std::make_unique<AdBlockLocalhostFiltersProvider>();
  }

  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      default_engine_.get(), AdBlockFiltersProviderManager::GetInstance(),
      resource_provider_.get(), GetTaskRunner(), true);

  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          additional_filters_engine_.get(),
          AdBlockFiltersProviderManager::GetInstance(),
          resource_provider_.get(), GetTaskRunner(), true);
}

AdBlockService::~AdBlockService() = default;

void AdBlockService::EnableTag(const std::string& tag, bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Tags only need to be modified for the default engine.
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::EnableTag,
                     base::Unretained(default_engine_.get()), tag, enabled));
}

void AdBlockService::AddUserCosmeticFilter(const std::string& filter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  custom_filters_provider_->AddUserCosmeticFilter(filter);
}

void AdBlockService::GetDebugInfoAsync(GetDebugInfoCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // base::Unretained() is safe because |default_engine_| is deleted
  // on the same sequence. See docs/threading_and_tasks_testing.md for
  // explanations.
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::GetDebugInfo,
                     base::Unretained(default_engine_.get())),
      base::BindOnce(&AdBlockService::OnGetDebugInfoFromDefaultEngine,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AdBlockService::DiscardRegex(uint64_t regex_id) {
  // Dispatch to both default & additional engines, ids are unique.
  GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AdBlockEngine::DiscardRegex,
                                default_engine_->AsWeakPtr(), regex_id));
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::DiscardRegex,
                     additional_filters_engine_->AsWeakPtr(), regex_id));
}

void AdBlockService::SetupDiscardPolicy(
    const adblock::RegexManagerDiscardPolicy& policy) {
  GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AdBlockEngine::SetupDiscardPolicy,
                                default_engine_->AsWeakPtr(), policy));
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::SetupDiscardPolicy,
                     additional_filters_engine_->AsWeakPtr(), policy));
}

base::SequencedTaskRunner* AdBlockService::GetTaskRunner() {
  return task_runner_.get();
}

void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockCookieListOptInShown, false);
  registry->RegisterBooleanPref(prefs::kAdBlockCookieListSettingTouched, false);
  registry->RegisterBooleanPref(
      prefs::kAdBlockMobileNotificationsListSettingTouched, false);
  registry->RegisterStringPref(prefs::kAdBlockCustomFilters, std::string());
  registry->RegisterDictionaryPref(prefs::kAdBlockRegionalFilters);
  registry->RegisterDictionaryPref(prefs::kAdBlockListSubscriptions);
  registry->RegisterBooleanPref(prefs::kAdBlockCheckedDefaultRegion, false);
  registry->RegisterBooleanPref(prefs::kAdBlockCheckedAllDefaultRegions, false);
}

AdBlockResourceProvider* AdBlockService::resource_provider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return resource_provider_.get();
}

void AdBlockService::UseSourceProviderForTest(
    AdBlockFiltersProvider* source_provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      default_engine_.get(), source_provider, resource_provider_.get(),
      GetTaskRunner());
}

void AdBlockService::UseCustomSourceProviderForTest(
    AdBlockFiltersProvider* source_provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          additional_filters_engine_.get(), source_provider,
          resource_provider_.get(), GetTaskRunner());
}

void AdBlockService::OnGetDebugInfoFromDefaultEngine(
    GetDebugInfoCallback callback,
    base::Value::Dict default_engine_debug_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // base::Unretained() is safe because |additional_filters_engine_| is deleted
  // on the same sequence. See docs/threading_and_tasks_testing.md for
  // explanations.
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::GetDebugInfo,
                     base::Unretained(additional_filters_engine_.get())),
      base::BindOnce(std::move(callback),
                     std::move(default_engine_debug_info)));
}

void AdBlockService::TagExistsForTest(const std::string& tag,
                                      base::OnceCallback<void(bool)> cb) {
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AdBlockEngine::TagExists,
                     base::Unretained(default_engine_.get()), tag),
      std::move(cb));
}

}  // namespace brave_shields
