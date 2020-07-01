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
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"
#include "components/prefs/pref_service.h"

namespace brave_shields {

std::string AdBlockRegionalService::g_ad_block_regional_component_id_;  // NOLINT
std::string
    AdBlockRegionalService::g_ad_block_regional_component_base64_public_key_;  // NOLINT

AdBlockRegionalService::AdBlockRegionalService(
    const std::string& uuid,
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate),
      uuid_(uuid) {
}

AdBlockRegionalService::~AdBlockRegionalService() {
}

bool AdBlockRegionalService::Init() {
  AdBlockBaseService::Init();
  std::vector<adblock::FilterList>&  region_lists =
    adblock::FilterList::GetRegionalLists();
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
      install_dir.AppendASCII(std::string("rs-") + uuid_)
          .AddExtension(FILE_PATH_LITERAL(".dat"));
  GetDATFileData(dat_file_path);
  base::FilePath resources_file_path =
      install_dir.AppendASCII(kAdBlockResourcesFilename);

  base::PostTaskAndReplyWithResult(
      GetTaskRunner().get(), FROM_HERE,
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     resources_file_path),
      base::BindOnce(&AdBlockRegionalService::OnResourcesFileDataReady,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockRegionalService::OnResourcesFileDataReady(
    const std::string& resources) {
  g_brave_browser_process->ad_block_regional_service_manager()->AddResources(
      resources);
}

// static
void AdBlockRegionalService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_ad_block_regional_component_id_ = component_id;
  g_ad_block_regional_component_base64_public_key_ =
      component_base64_public_key;
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockRegionalService> AdBlockRegionalServiceFactory(
    const std::string& uuid,
    brave_component_updater::BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockRegionalService>(uuid, delegate);
}

}  // namespace brave_shields
