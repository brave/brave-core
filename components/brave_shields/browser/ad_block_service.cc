/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_default_resource_provider.h"
#include "brave/components/brave_shields/browser/ad_block_engine.h"
#include "brave/components/brave_shields/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/common/adblock_domain_resolver.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/origin.h"

namespace {

const char kAdBlockComponentName[] = "Brave Ad Block Updater";
const char kAdBlockComponentId[] = "iodkpdagapdfkphljnddpjlldadblomo";
const char kAdBlockComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsD/B/MGdz0gh7WkcFARn"
    "ZTBX9KAw2fuGeogijoI+fET38IK0L+P/trCT2NshqhRNmrDpLzV2+Dmes6PvkA+O"
    "dQkUV6VbChJG+baTfr3Oo5PdE0WxmP9Xh8XD7p85DQrk0jJilKuElxpK7Yq0JhcT"
    "Sc3XNHeTwBVqCnHwWZZ+XysYQfjuDQ0MgQpS/s7U04OZ63NIPe/iCQm32stvS/pE"
    "ya7KdBZXgRBQ59U6M1n1Ikkp3vfECShbBld6VrrmNrl59yKWlEPepJ9oqUc2Wf2M"
    "q+SDNXROG554RnU4BnDJaNETTkDTZ0Pn+rmLmp1qY5Si0yGsfHkrv3FS3vdxVozO"
    "PQIDAQAB";

std::string g_ad_block_component_id_(kAdBlockComponentId);
std::string g_ad_block_component_base64_public_key_(
    kAdBlockComponentBase64PublicKey);

}  // namespace

namespace brave_shields {

AdBlockService::SourceProviderObserver::SourceProviderObserver(
    AdBlockEngine* adblock_engine,
    AdBlockFiltersProvider* filters_provider,
    AdBlockResourceProvider* resource_provider,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : adblock_engine_(adblock_engine),
      filters_provider_(filters_provider),
      resource_provider_(resource_provider),
      task_runner_(task_runner) {
  filters_provider_->AddObserver(this);
  filters_provider_->LoadDAT(
      base::BindOnce(&AdBlockService::SourceProviderObserver::OnDATLoaded,
                     weak_factory_.GetWeakPtr()));
}

AdBlockService::SourceProviderObserver::~SourceProviderObserver() {
  filters_provider_->RemoveObserver(this);
  resource_provider_->RemoveObserver(this);
}

void AdBlockService::SourceProviderObserver::OnChanged() {
  filters_provider_->LoadDAT(
      base::BindOnce(&AdBlockService::SourceProviderObserver::OnDATLoaded,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::OnDATLoaded(
    bool deserialize,
    const DATFileDataBuffer& dat_buf) {
  deserialize_ = deserialize;
  dat_buf_ = std::move(dat_buf);
  // multiple AddObserver calls are ignored
  resource_provider_->AddObserver(this);
  resource_provider_->LoadResources(base::BindOnce(
      &SourceProviderObserver::OnResourcesLoaded, weak_factory_.GetWeakPtr()));
}

void AdBlockService::SourceProviderObserver::OnResourcesLoaded(
    const std::string& resources_json) {
  if (dat_buf_.empty()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AdBlockEngine::UseResources,
                       adblock_engine_->AsWeakPtr(), resources_json));
  } else {
    auto engine_load_callback = base::BindOnce(
        [](base::WeakPtr<AdBlockEngine> engine, bool deserialize,
           DATFileDataBuffer dat_buf, const std::string& resources_json) {
          if (engine) {
            engine->Load(deserialize, std::move(dat_buf), resources_json);
          }
        },
        adblock_engine_->AsWeakPtr(), deserialize_, std::move(dat_buf_),
        resources_json);
    task_runner_->PostTask(FROM_HERE, std::move(engine_load_callback));
  }
}

void AdBlockService::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* mock_data_url,
    std::string* rewritten_url) {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());

  GURL request_url;

  if (aggressive_blocking ||
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) ||
      !SameDomainOrHost(
          url, url::Origin::CreateFromNormalizedTuple("https", tab_host, 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    request_url =
        rewritten_url && !rewritten_url->empty() ? GURL(*rewritten_url) : url;
    default_engine_->ShouldStartRequest(
        request_url, resource_type, tab_host, aggressive_blocking,
        did_match_rule, did_match_exception, did_match_important, mock_data_url,
        rewritten_url);
    if (did_match_important && *did_match_important) {
      return;
    }
  }

  request_url =
      rewritten_url && !rewritten_url->empty() ? GURL(*rewritten_url) : url;
  additional_filters_engine_->ShouldStartRequest(
      request_url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, mock_data_url, rewritten_url);
}

absl::optional<std::string> AdBlockService::GetCspDirectives(
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

AdBlockRegionalServiceManager* AdBlockService::regional_service_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return regional_service_manager_.get();
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
      default_engine_(std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
          new AdBlockEngine(),
          base::OnTaskRunnerDeleter(GetTaskRunner()))),
      additional_filters_engine_(
          std::unique_ptr<AdBlockEngine, base::OnTaskRunnerDeleter>(
              new AdBlockEngine(),
              base::OnTaskRunnerDeleter(GetTaskRunner()))) {
  // Initializes adblock-rust's domain resolution implementation
  adblock::SetDomainResolver(AdBlockServiceDomainResolver);

  if (base::FeatureList::IsEnabled(
          features::kAdblockOverrideRegexDiscardPolicy)) {
    adblock::RegexManagerDiscardPolicy policy;
    policy.cleanup_interval_sec =
        features::kAdblockOverrideRegexDiscardPolicyCleanupIntervalSec.Get();
    policy.discard_unused_sec =
        features::kAdblockOverrideRegexDiscardPolicyDiscardUnusedSec.Get();
    SetupDiscardPolicy(policy);
  }

  resource_provider_ = std::make_unique<AdBlockDefaultResourceProvider>(
      component_update_service_);
  filter_list_catalog_provider_ =
      std::make_unique<AdBlockFilterListCatalogProvider>(
          component_update_service_);

  default_filters_provider_ = std::make_unique<AdBlockComponentFiltersProvider>(
      component_update_service_, g_ad_block_component_id_,
      g_ad_block_component_base64_public_key_, kAdBlockComponentName);
  regional_service_manager_ = std::make_unique<AdBlockRegionalServiceManager>(
      local_state_, locale_, component_update_service_,
      filter_list_catalog_provider_.get());
  subscription_service_manager_ =
      std::make_unique<AdBlockSubscriptionServiceManager>(
          local_state_, std::move(subscription_download_manager_getter_),
          profile_dir_);
  custom_filters_provider_ =
      std::make_unique<AdBlockCustomFiltersProvider>(local_state_);

  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      default_engine_.get(), default_filters_provider_.get(),
      resource_provider_.get(), GetTaskRunner());
  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          additional_filters_engine_.get(),
          AdBlockFiltersProviderManager::GetInstance(),
          resource_provider_.get(), GetTaskRunner());
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
}

AdBlockResourceProvider* AdBlockService::resource_provider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return resource_provider_.get();
}

void AdBlockService::UseSourceProvidersForTest(
    AdBlockFiltersProvider* source_provider,
    AdBlockResourceProvider* resource_provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  default_service_observer_ = std::make_unique<SourceProviderObserver>(
      default_engine_.get(), source_provider, resource_provider,
      GetTaskRunner());
}

void AdBlockService::UseCustomSourceProvidersForTest(
    AdBlockFiltersProvider* source_provider,
    AdBlockResourceProvider* resource_provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  additional_filters_service_observer_ =
      std::make_unique<SourceProviderObserver>(
          additional_filters_engine_.get(), source_provider, resource_provider,
          GetTaskRunner());
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

// static
void SetDefaultAdBlockComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_ad_block_component_id_ = component_id;
  g_ad_block_component_base64_public_key_ = component_base64_public_key;
}

}  // namespace brave_shields
