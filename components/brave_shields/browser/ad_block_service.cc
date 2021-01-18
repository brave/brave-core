/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "base/json/json_writer.h"

#define DAT_FILE "rs-ABPFilterParserData.dat"
#define REGIONAL_CATALOG "regional_catalog.json"

namespace brave_shields {

namespace {

std::string GetTagFromPrefName(const std::string& pref_name) {
  if (pref_name == kFBEmbedControlType) {
    return brave_shields::kFacebookEmbeds;
  }
  if (pref_name == kTwitterEmbedControlType) {
    return brave_shields::kTwitterEmbeds;
  }
  if (pref_name == kLinkedInEmbedControlType) {
    return brave_shields::kLinkedInEmbeds;
  }
  return "";
}

// Extracts the start and end characters of a domain from a hostname.
// Required for correct functionality of adblock-rust.
void AdBlockServiceDomainResolver(const char* host,
                                  uint32_t* start,
                                  uint32_t* end) {
  const auto host_str = std::string(host);
  const auto domain = net::registry_controlled_domains::GetDomainAndRegistry(
      host_str,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
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

bool AdBlockService::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool* did_match_exception,
    std::string* mock_data_url) {
  if (!AdBlockBaseService::ShouldStartRequest(
          url, resource_type, tab_host, did_match_exception, mock_data_url)) {
    return false;
  }
  if (did_match_exception && *did_match_exception) {
    return true;
  }

  if (!regional_service_manager()->ShouldStartRequest(
          url, resource_type, tab_host, did_match_exception, mock_data_url)) {
    return false;
  }
  if (did_match_exception && *did_match_exception) {
    return true;
  }

  if (!custom_filters_service()->ShouldStartRequest(
          url, resource_type, tab_host, did_match_exception, mock_data_url)) {
    return false;
  }
  if (did_match_exception && *did_match_exception) {
    return true;
  }

  return true;
}

base::Optional<base::Value> AdBlockService::UrlCosmeticResources(
    const std::string& url) {
  base::Optional<base::Value> resources =
      AdBlockBaseService::UrlCosmeticResources(url);

  if (!resources || !resources->is_dict()) {
    return resources;
  }

  base::Optional<base::Value> regional_resources =
      regional_service_manager()->UrlCosmeticResources(url);

  if (regional_resources && regional_resources->is_dict()) {
    MergeResourcesInto(std::move(*regional_resources), &*resources,
                       /*force_hide=*/false);
  }

  base::Optional<base::Value> custom_resources =
      custom_filters_service()->UrlCosmeticResources(url);

  if (custom_resources && custom_resources->is_dict()) {
    MergeResourcesInto(std::move(*custom_resources), &*resources,
                       /*force_hide=*/true);
  }

  return resources;
}

base::Optional<base::Value> AdBlockService::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  base::Optional<base::Value> hide_selectors =
      AdBlockBaseService::HiddenClassIdSelectors(
          classes, ids, exceptions);

  base::Optional<base::Value> regional_selectors =
      regional_service_manager()
          ->HiddenClassIdSelectors(classes, ids, exceptions);

  base::Optional<base::Value> custom_selectors =
      custom_filters_service()
          ->HiddenClassIdSelectors(classes, ids, exceptions);

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

  if (!hide_selectors || !hide_selectors->is_list())
    hide_selectors = base::ListValue();

  if (custom_selectors && custom_selectors->is_list())
    hide_selectors->Append(std::move(*custom_selectors));

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

AdBlockService::AdBlockService(
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate), component_delegate_(delegate) {
}

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

///////////////////////////////////////////////////////////////////////////////

// The Adblock service factory.
std::unique_ptr<AdBlockService> AdBlockServiceFactory(
    brave_component_updater::BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockService>(delegate);
}

void RegisterPrefsForAdBlockService(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kAdBlockCustomFilters, std::string());
  registry->RegisterDictionaryPref(kAdBlockRegionalFilters);
  registry->RegisterBooleanPref(kAdBlockCheckedDefaultRegion, false);
}

AdBlockPrefService::AdBlockPrefService(PrefService* prefs) : prefs_(prefs) {
  pref_change_registrar_.reset(new PrefChangeRegistrar());
  pref_change_registrar_->Init(prefs_);
  pref_change_registrar_->Add(
      kFBEmbedControlType,
      base::BindRepeating(&AdBlockPrefService::OnPreferenceChanged,
                          base::Unretained(this), kFBEmbedControlType));
  pref_change_registrar_->Add(
      kTwitterEmbedControlType,
      base::BindRepeating(&AdBlockPrefService::OnPreferenceChanged,
                          base::Unretained(this), kTwitterEmbedControlType));
  pref_change_registrar_->Add(
      kLinkedInEmbedControlType,
      base::BindRepeating(&AdBlockPrefService::OnPreferenceChanged,
                          base::Unretained(this), kLinkedInEmbedControlType));
  OnPreferenceChanged(kFBEmbedControlType);
  OnPreferenceChanged(kTwitterEmbedControlType);
  OnPreferenceChanged(kLinkedInEmbedControlType);
}

AdBlockPrefService::~AdBlockPrefService() = default;

void AdBlockPrefService::OnPreferenceChanged(const std::string& pref_name) {
  std::string tag = GetTagFromPrefName(pref_name);
  if (tag.length() == 0) {
    return;
  }
  bool enabled = prefs_->GetBoolean(pref_name);
  g_brave_browser_process->ad_block_service()->EnableTag(tag, enabled);
  g_brave_browser_process->ad_block_regional_service_manager()->EnableTag(
      tag, enabled);
  g_brave_browser_process->ad_block_custom_filters_service()->EnableTag(
      tag, enabled);
}

}  // namespace brave_shields
