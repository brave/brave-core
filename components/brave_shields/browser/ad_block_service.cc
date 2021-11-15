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
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
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
#define REGIONAL_CATALOG "regional_catalog.json"

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

std::string AdBlockService::g_ad_block_component_id_(kAdBlockComponentId);
std::string AdBlockService::g_ad_block_component_base64_public_key_(
    kAdBlockComponentBase64PublicKey);

void AdBlockService::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* replacement_url) {
  if (!IsInitialized())
    return;

  if (aggressive_blocking ||
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) ||
      !SameDomainOrHost(
          url, url::Origin::CreateFromNormalizedTuple("https", tab_host, 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    AdBlockBaseService::ShouldStartRequest(
        url, resource_type, tab_host, aggressive_blocking, did_match_rule,
        did_match_exception, did_match_important, replacement_url);
    if (did_match_important && *did_match_important) {
      return;
    }
  }

  regional_service_manager()->ShouldStartRequest(
      url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, replacement_url);
  if (did_match_important && *did_match_important) {
    return;
  }

  subscription_service_manager()->ShouldStartRequest(
      url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, replacement_url);
  if (did_match_important && *did_match_important) {
    return;
  }

  custom_filters_service()->ShouldStartRequest(
      url, resource_type, tab_host, aggressive_blocking, did_match_rule,
      did_match_exception, did_match_important, replacement_url);
}

absl::optional<std::string> AdBlockService::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  auto csp_directives =
      AdBlockBaseService::GetCspDirectives(url, resource_type, tab_host);

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
      AdBlockBaseService::UrlCosmeticResources(url);

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

absl::optional<base::Value> AdBlockService::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  absl::optional<base::Value> hide_selectors =
      AdBlockBaseService::HiddenClassIdSelectors(classes, ids, exceptions);

  absl::optional<base::Value> regional_selectors =
      regional_service_manager()->HiddenClassIdSelectors(classes, ids,
                                                         exceptions);

  if (hide_selectors && hide_selectors->is_list()) {
    if (regional_selectors && regional_selectors->is_list()) {
      for (auto i = regional_selectors->GetList().begin();
           i < regional_selectors->GetList().end(); i++) {
        hide_selectors->Append(std::move(*i));
      }
    }
  } else {
    hide_selectors = std::move(regional_selectors);
  }

  absl::optional<base::Value> custom_selectors =
      custom_filters_service()->HiddenClassIdSelectors(classes, ids,
                                                       exceptions);

  absl::optional<base::Value> subscription_selectors =
      subscription_service_manager()->HiddenClassIdSelectors(classes, ids,
                                                             exceptions);

  if (custom_selectors && custom_selectors->is_list()) {
    if (subscription_selectors && subscription_selectors->is_list()) {
      for (auto i = subscription_selectors->GetList().begin();
           i < subscription_selectors->GetList().end(); i++) {
        custom_selectors->Append(std::move(*i));
      }
    }
  } else {
    custom_selectors = std::move(subscription_selectors);
  }

  if (!hide_selectors || !hide_selectors->is_list())
    hide_selectors = base::ListValue();

  if (custom_selectors && custom_selectors->is_list()) {
    for (auto i = custom_selectors->GetList().begin();
         i < custom_selectors->GetList().end(); i++) {
      hide_selectors->Append(std::move(*i));
    }
  }

  return hide_selectors;
}

AdBlockRegionalServiceManager* AdBlockService::regional_service_manager() {
  if (!regional_service_manager_)
    regional_service_manager_ =
        brave_shields::AdBlockRegionalServiceManagerFactory(
            component_delegate_);
  return regional_service_manager_.get();
}

brave_shields::AdBlockCustomFiltersService*
AdBlockService::custom_filters_service() {
  if (!custom_filters_service_)
    custom_filters_service_ =
        brave_shields::AdBlockCustomFiltersServiceFactory(component_delegate_);
  return custom_filters_service_.get();
}

brave_shields::AdBlockSubscriptionServiceManager*
AdBlockService::subscription_service_manager() {
  return subscription_service_manager_.get();
}

AdBlockService::AdBlockService(
    brave_component_updater::BraveComponent::Delegate* delegate,
    std::unique_ptr<AdBlockSubscriptionServiceManager>
        subscription_service_manager)
    : AdBlockBaseService(delegate),
      component_delegate_(delegate),
      subscription_service_manager_(std::move(subscription_service_manager)) {}

AdBlockService::~AdBlockService() {}

bool AdBlockService::Init() {
  // Initializes adblock-rust's domain resolution implementation
  adblock::SetDomainResolver(AdBlockServiceDomainResolver);

  if (!AdBlockBaseService::Init())
    return false;

  Register(kAdBlockComponentName, g_ad_block_component_id_,
           g_ad_block_component_base64_public_key_);
  return true;
}

void AdBlockService::OnComponentReady(const std::string& component_id,
                                      const base::FilePath& install_dir,
                                      const std::string& manifest) {
  // Regional service manager depends on regional catalog loading
  custom_filters_service()->Start();
  subscription_service_manager()->Start();

  base::FilePath dat_file_path = install_dir.AppendASCII(DAT_FILE);
  GetDATFileData(dat_file_path);

  base::FilePath regional_catalog_file_path =
      install_dir.AppendASCII(REGIONAL_CATALOG);

  base::FilePath resources_file_path =
      install_dir.AppendASCII(kAdBlockResourcesFilename);
  base::PostTaskAndReplyWithResult(
      GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     resources_file_path),
      base::BindOnce(&AdBlockService::OnResourcesFileDataReady,
                     weak_factory_.GetWeakPtr()));
  base::PostTaskAndReplyWithResult(
      GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     regional_catalog_file_path),
      base::BindOnce(&AdBlockService::OnRegionalCatalogFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockService::OnResourcesFileDataReady(const std::string& resources) {
  AddResources(resources);
  custom_filters_service()->AddResources(resources);
}

void AdBlockService::OnRegionalCatalogFileDataReady(
    const std::string& catalog_json) {
  regional_service_manager()->SetRegionalCatalog(
      RegionalCatalogFromJSON(catalog_json));
  regional_service_manager()->Start();
}

// static
void AdBlockService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_ad_block_component_id_ = component_id;
  g_ad_block_component_base64_public_key_ = component_base64_public_key;
}

void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockCookieListSettingTouched, false);
  registry->RegisterStringPref(prefs::kAdBlockCustomFilters, std::string());
  registry->RegisterDictionaryPref(prefs::kAdBlockRegionalFilters);
  registry->RegisterDictionaryPref(prefs::kAdBlockListSubscriptions);
  registry->RegisterBooleanPref(prefs::kAdBlockCheckedDefaultRegion, false);
}

}  // namespace brave_shields
