/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/vendor/ad-block/ad_block_client.h"
#include "brave/vendor/ad-block/data_file_version.h"
#include "brave/vendor/ad-block/lists/regions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace {

std::vector<FilterList>::const_iterator
FindFilterListByLocale(const std::string& locale) {
  std::string adjusted_locale;
  std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    adjusted_locale = locale;
  } else {
    adjusted_locale = locale.substr(0, loc);
  }
  adjusted_locale = base::ToLowerASCII(adjusted_locale);
  return std::find_if(region_lists.begin(), region_lists.end(),
      [&adjusted_locale](const FilterList& filter_list) {
        return std::find_if(filter_list.langs.begin(),
                            filter_list.langs.end(),
                            [adjusted_locale](const std::string& lang) {
                              return lang == adjusted_locale;
                            }) != filter_list.langs.end();
      });
}

}  // namespace

namespace brave_shields {

std::string AdBlockRegionalService::g_ad_block_regional_component_id_;  // NOLINT
std::string
    AdBlockRegionalService::g_ad_block_regional_component_base64_public_key_;  // NOLINT
std::string AdBlockRegionalService::g_ad_block_regional_dat_file_version_(
    base::NumberToString(DATA_FILE_VERSION));

AdBlockRegionalService::AdBlockRegionalService() {
}

AdBlockRegionalService::~AdBlockRegionalService() {
}

bool AdBlockRegionalService::UnregisterComponentByLocale(
    const std::string& locale) {
  auto it = FindFilterListByLocale(locale);
  if (it == region_lists.end())
    return false;
  return Unregister(it->component_id);
}

bool AdBlockRegionalService::Init() {
  AdBlockBaseService::Init();
  auto it =
      FindFilterListByLocale(g_brave_browser_process->GetApplicationLocale());
  if (it == region_lists.end())
    return false;

  uuid_ = it->uuid;
  title_ = it->title;

  Register(it->title,
           !g_ad_block_regional_component_id_.empty()
               ? g_ad_block_regional_component_id_
               : it->component_id,
           !g_ad_block_regional_component_base64_public_key_.empty()
               ? g_ad_block_regional_component_base64_public_key_
               : it->base64_public_key);

  return true;
}

void AdBlockRegionalService::OnComponentRegistered(
    const std::string& component_id) {
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  std::string ad_block_current_region = prefs->GetString(kAdBlockCurrentRegion);
  std::string locale = g_brave_browser_process->GetApplicationLocale();
  if (!ad_block_current_region.empty() && ad_block_current_region != locale)
    UnregisterComponentByLocale(ad_block_current_region);
  prefs->SetString(kAdBlockCurrentRegion, locale);
  AdBlockBaseService::OnComponentRegistered(component_id);
}

void AdBlockRegionalService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::FilePath dat_file_path =
      install_dir.AppendASCII(g_ad_block_regional_dat_file_version_)
          .AppendASCII(uuid_)
          .AddExtension(FILE_PATH_LITERAL(".dat"));
  AdBlockBaseService::GetDATFileData(dat_file_path);
}

// static
bool AdBlockRegionalService::IsSupportedLocale(const std::string& locale) {
  return (FindFilterListByLocale(locale) != region_lists.end());
}

// static
std::unique_ptr<base::ListValue> AdBlockRegionalService::GetRegionalLists() {
  auto list_value = std::make_unique<base::ListValue>();
  for (const auto& region_list : region_lists) {
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetString("uuid", region_list.uuid);
    dict->SetString("url", region_list.url);
    dict->SetString("title", region_list.title);
    dict->SetString("support_url", region_list.support_url);
    dict->SetString("component_id", region_list.component_id);
    dict->SetString("base64_public_key", region_list.base64_public_key);
    dict->SetBoolean("enabled", false);
    list_value->Append(std::move(dict));
  }

  return list_value;
}

// static
void AdBlockRegionalService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_ad_block_regional_component_id_ = component_id;
  g_ad_block_regional_component_base64_public_key_ =
      component_base64_public_key;
}

// static
void AdBlockRegionalService::SetDATFileVersionForTest(
  const std::string& dat_file_version) {
  g_ad_block_regional_dat_file_version_ = dat_file_version;
}

scoped_refptr<base::SequencedTaskRunner>
AdBlockRegionalService::GetTaskRunner() {
  // We share the same task runner for all ad-block and TP code
  return g_brave_browser_process->ad_block_service()->GetTaskRunner();
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<AdBlockRegionalService> AdBlockRegionalServiceFactory() {
  return std::make_unique<AdBlockRegionalService>();
}

}  // namespace brave_shields
