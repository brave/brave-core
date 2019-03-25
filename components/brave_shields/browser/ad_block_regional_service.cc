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
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/vendor/ad-block/ad_block_client.h"
#include "brave/vendor/ad-block/data_file_version.h"
#include "brave/vendor/ad-block/lists/regions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

std::string AdBlockRegionalService::g_ad_block_regional_component_id_;  // NOLINT
std::string
    AdBlockRegionalService::g_ad_block_regional_component_base64_public_key_;  // NOLINT
std::string AdBlockRegionalService::g_ad_block_regional_dat_file_version_(
    base::NumberToString(DATA_FILE_VERSION));

AdBlockRegionalService::AdBlockRegionalService(const std::string& uuid)
    : uuid_(uuid) {
}

AdBlockRegionalService::~AdBlockRegionalService() {
}

bool AdBlockRegionalService::Init() {
  AdBlockBaseService::Init();
  auto it = brave_shields::FindAdBlockFilterListByUUID(region_lists, uuid_);
  if (it == region_lists.end())
    return false;

  Register(it->title,
           !g_ad_block_regional_component_id_.empty()
               ? g_ad_block_regional_component_id_
               : it->component_id,
           !g_ad_block_regional_component_base64_public_key_.empty()
               ? g_ad_block_regional_component_base64_public_key_
               : it->base64_public_key);

  title_ = it->title;

  return true;
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

std::unique_ptr<AdBlockRegionalService> AdBlockRegionalServiceFactory(
    const std::string& uuid) {
  return std::make_unique<AdBlockRegionalService>(uuid);
}

}  // namespace brave_shields
