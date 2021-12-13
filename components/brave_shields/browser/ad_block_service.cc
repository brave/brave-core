/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_source_provider.h"
#include "brave/components/brave_shields/browser/ad_block_default_source_provider.h"
#include "brave/components/brave_shields/browser/ad_block_engine.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/origin.h"

#define DAT_FILE "rs-ABPFilterParserData.dat"

namespace brave_shields {

namespace {

// Extracts the start and end characters of a domain from a hostname.
// Required for correct functionality of adblock-rust.
void AdBlockServiceDomainResolver(const char* host,
                                  uint32_t* start,
                                  uint32_t* end) {
  const auto host_str = std::string(host);
  const auto domain = net::registry_controlled_domains::GetDomainAndRegistry(
      host_str, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  const size_t match = host_str.rfind(domain);
  if (match != std::string::npos) {
    *start = match;
    *end = match + domain.length();
  } else {
    *start = 0;
    *end = host_str.length();
  }
}

}  // namespace

void AdBlockService::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* mock_data_url) {
  if (aggressive_blocking ||
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) ||
      !SameDomainOrHost(
          url, url::Origin::CreateFromNormalizedTuple("https", tab_host, 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    default_service()->ShouldStartRequest(
        url, resource_type, tab_host, aggressive_blocking, did_match_rule,
        did_match_exception, did_match_important, mock_data_url);
    if (did_match_important && *did_match_important) {
      return;
    }
  }

  regional_service_manager()->ShouldStartRequest(
      url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, mock_data_url);
  if (did_match_important && *did_match_important) {
    return;
  }

  subscription_service_manager()->ShouldStartRequest(
      url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, mock_data_url);
  if (did_match_important && *did_match_important) {
    return;
  }

  custom_filters_service()->ShouldStartRequest(
      url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, mock_data_url);
}

absl::optional<std::string> AdBlockService::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  auto csp_directives =
      default_service()->GetCspDirectives(url, resource_type, tab_host);

  const auto regional_csp = regional_service_manager()->GetCspDirectives(
      url, resource_type, tab_host);
  MergeCspDirectiveInto(regional_csp, &csp_directives);

  const auto custom_csp =
      custom_filters_service()->GetCspDirectives(url, resource_type, tab_host);
  MergeCspDirectiveInto(custom_csp, &csp_directives);

  return csp_directives;
}

absl::optional<base::Value> AdBlockService::UrlCosmeticResources(
    const std::string& url) {
  absl::optional<base::Value> resources =
      default_service()->UrlCosmeticResources(url);

  if (!resources || !resources->is_dict()) {
    return resources;
  }

  absl::optional<base::Value> regional_resources =
      regional_service_manager()->UrlCosmeticResources(url);

  if (regional_resources && regional_resources->is_dict()) {
    MergeResourcesInto(std::move(*regional_resources), &*resources,
                       /*force_hide=*/false);
  }

  absl::optional<base::Value> custom_resources =
      custom_filters_service()->UrlCosmeticResources(url);

  if (custom_resources && custom_resources->is_dict()) {
    MergeResourcesInto(std::move(*custom_resources), &*resources,
                       /*force_hide=*/true);
  }

  absl::optional<base::Value> subscription_resources =
      subscription_service_manager()->UrlCosmeticResources(url);

  if (subscription_resources && subscription_resources->is_dict()) {
    MergeResourcesInto(std::move(*subscription_resources), &*resources,
                       /*force_hide=*/true);
  }

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
base::Value AdBlockService::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  base::Value hide_selectors =
      default_service()->HiddenClassIdSelectors(classes, ids, exceptions);

  base::Value regional_selectors =
      regional_service_manager()->HiddenClassIdSelectors(classes, ids,
                                                         exceptions);
  DCHECK(regional_selectors.is_list());

  base::Value custom_selectors =
      custom_filters_service()->HiddenClassIdSelectors(classes, ids,
                                                       exceptions);
  DCHECK(custom_selectors.is_list());

  base::Value subscription_selectors =
      subscription_service_manager()->HiddenClassIdSelectors(classes, ids,
                                                             exceptions);
  DCHECK(subscription_selectors.is_list());

  base::Value force_hide_selectors = std::move(regional_selectors);

  for (auto i = custom_selectors.GetList().begin();
       i < custom_selectors.GetList().end(); i++) {
    force_hide_selectors.Append(std::move(*i));
  }

  for (auto i = subscription_selectors.GetList().begin();
       i < subscription_selectors.GetList().end(); i++) {
    force_hide_selectors.Append(std::move(*i));
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("hide_selectors", std::move(hide_selectors));
  result.SetKey("force_hide_selectors", std::move(force_hide_selectors));
  return result;
}

AdBlockRegionalServiceManager* AdBlockService::regional_service_manager() {
  if (!regional_service_manager_) {
    regional_service_manager_ =
        brave_shields::AdBlockRegionalServiceManagerFactory(
            local_state_, locale_, component_update_service_, GetTaskRunner());
    regional_service_manager_->Init(resource_provider(),
                                    regional_catalog_provider());
  }
  return regional_service_manager_.get();
}

AdBlockEngine* AdBlockService::default_service() {
  if (!default_service_) {
    default_service_ = std::make_unique<AdBlockEngine>(GetTaskRunner());
    default_service_->Init(default_source_provider_.get(),
                           default_source_provider_.get());
  }
  return default_service_.get();
}

AdBlockResourceProvider* AdBlockService::resource_provider() {
  return default_source_provider_.get();
}

AdBlockRegionalCatalogProvider* AdBlockService::regional_catalog_provider() {
  return default_source_provider_.get();
}

AdBlockEngine* AdBlockService::custom_filters_service() {
  if (!custom_filters_service_) {
    custom_filters_service_ = std::make_unique<AdBlockEngine>(GetTaskRunner());
    custom_filters_service_->Init(custom_filters_source_provider_.get(),
                                  resource_provider());
  }
  return custom_filters_service_.get();
}

brave_shields::AdBlockCustomFiltersSourceProvider*
AdBlockService::custom_filters_source_provider() {
  return custom_filters_source_provider_.get();
}

brave_shields::AdBlockSubscriptionServiceManager*
AdBlockService::subscription_service_manager() {
  if (!subscription_service_manager_->IsInitialized()) {
    subscription_service_manager_->Init(resource_provider());
  }
  return subscription_service_manager_.get();
}

AdBlockService::AdBlockService(
    PrefService* local_state,
    std::string locale,
    component_updater::ComponentUpdateService* cus,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    std::unique_ptr<AdBlockSubscriptionServiceManager>
        subscription_service_manager)
    : local_state_(local_state),
      locale_(locale),
      component_update_service_(cus),
      task_runner_(task_runner),
      subscription_service_manager_(std::move(subscription_service_manager)) {
  default_source_provider_ =
      std::make_unique<brave_shields::AdBlockDefaultSourceProvider>(
          component_update_service_);
  custom_filters_source_provider_ =
      std::make_unique<brave_shields::AdBlockCustomFiltersSourceProvider>(
          local_state_);
}

AdBlockService::~AdBlockService() {}

bool AdBlockService::Start() {
  // Initializes adblock-rust's domain resolution implementation
  adblock::SetDomainResolver(AdBlockServiceDomainResolver);

  // Initialize each service:
  default_service();
  custom_filters_service();
  regional_service_manager();
  subscription_service_manager();

  return true;
}

void AdBlockService::EnableTag(const std::string& tag, bool enabled) {
  // Tags only need to be modified for the default engine.
  default_service()->EnableTag(tag, enabled);
}

void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockCookieListSettingTouched, false);
  registry->RegisterStringPref(prefs::kAdBlockCustomFilters, std::string());
  registry->RegisterDictionaryPref(prefs::kAdBlockRegionalFilters);
  registry->RegisterDictionaryPref(prefs::kAdBlockListSubscriptions);
  registry->RegisterBooleanPref(prefs::kAdBlockCheckedDefaultRegion, false);
}

void AdBlockService::UseSourceProvidersForTest(
    AdBlockSourceProvider* source_provider,
    AdBlockResourceProvider* resource_provider) {
  default_service_->Init(source_provider, resource_provider);
}

void AdBlockService::UseCustomSourceProvidersForTest(
    AdBlockSourceProvider* source_provider,
    AdBlockResourceProvider* resource_provider) {
  custom_filters_service_->Init(source_provider, resource_provider);
}

bool AdBlockService::TagExistsForTest(const std::string& tag) {
  return default_service()->TagExists(tag);
}

}  // namespace brave_shields
