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

}  // namespace

std::string AdBlockService::g_ad_block_component_id_(kAdBlockComponentId);
std::string AdBlockService::g_ad_block_component_base64_public_key_(
    kAdBlockComponentBase64PublicKey);

AdBlockService::AdBlockService(
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate) {
}

AdBlockService::~AdBlockService() {}

bool AdBlockService::Init() {
  if (!AdBlockBaseService::Init())
    return false;

  Register(kAdBlockComponentName, g_ad_block_component_id_,
           g_ad_block_component_base64_public_key_);
  return true;
}

void AdBlockService::OnComponentReady(const std::string& component_id,
                                      const base::FilePath& install_dir,
                                      const std::string& manifest) {
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
  g_brave_browser_process->ad_block_service()->AddResources(resources);
  g_brave_browser_process->ad_block_custom_filters_service()->AddResources(
      resources);
}

void AdBlockService::OnRegionalCatalogFileDataReady(
    const std::string& catalog_json) {
  g_brave_browser_process->ad_block_regional_service_manager()
      ->SetRegionalCatalog(RegionalCatalogFromJSON(catalog_json));
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
