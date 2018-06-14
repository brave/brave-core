/* This Source Code Form is subject to the terms of the Mozilla Public
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
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/vendor/ad-block/ad_block_client.h"
#include "brave/vendor/ad-block/data_file_version.h"
#include "brave/vendor/ad-block/lists/regions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace {

std::vector<FilterList>::const_iterator FindFilterListByLocale(const std::string& locale) {
  return std::find_if(region_lists.begin(), region_lists.end(),
                      [&locale](const FilterList& filter_list) {
                        return std::find_if(filter_list.langs.begin(),
                                            filter_list.langs.end(),
                                            [locale](const std::string& lang) {
                                              return lang == locale;
                                            }) != filter_list.langs.end();
                      });
}

}  // namespace

namespace brave_shields {

std::string AdBlockRegionalService::g_ad_block_regional_component_id_;
std::string AdBlockRegionalService::g_ad_block_regional_component_base64_public_key_;
std::string AdBlockRegionalService::g_ad_block_regional_dat_file_version_(
    base::NumberToString(DATA_FILE_VERSION));

AdBlockRegionalService::AdBlockRegionalService() {
}

AdBlockRegionalService::~AdBlockRegionalService() {
}

bool AdBlockRegionalService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host) {
  return AdBlockBaseService::ShouldStartRequest(url, resource_type, tab_host);
}

bool AdBlockRegionalService::UnregisterComponentByLocale(const std::string& locale) {
  auto it = FindFilterListByLocale(locale);
  if (it == region_lists.end())
    return false;
  return Unregister(it->component_id);

}

bool AdBlockRegionalService::Init() {
  auto it =
      FindFilterListByLocale(g_brave_browser_process->GetApplicationLocale());
  if (it == region_lists.end())
    return false;

  uuid_ = it->uuid;

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
    const base::FilePath& install_dir) {
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
void AdBlockRegionalService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_ad_block_regional_component_id_ = component_id;
  g_ad_block_regional_component_base64_public_key_ = component_base64_public_key;
}

// static
void AdBlockRegionalService::SetDATFileVersionForTest(
  const std::string& dat_file_version) {
  g_ad_block_regional_dat_file_version_ = dat_file_version;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<AdBlockRegionalService> AdBlockRegionalServiceFactory() {
  return std::make_unique<AdBlockRegionalService>();
}

}  // namespace brave_shields
