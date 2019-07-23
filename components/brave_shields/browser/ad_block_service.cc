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
#include "brave/common/pref_names.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"
#include "components/prefs/pref_registry_simple.h"

#define DAT_FILE "rs-ABPFilterParserData.dat"

namespace brave_shields {

std::string AdBlockService::g_ad_block_component_id_(
    kAdBlockComponentId);
std::string AdBlockService::g_ad_block_component_base64_public_key_(
    kAdBlockComponentBase64PublicKey);

AdBlockService::AdBlockService(
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate) {
}

AdBlockService::~AdBlockService() {
}

bool AdBlockService::Init() {
  if (!AdBlockBaseService::Init())
    return false;

  Register(kAdBlockComponentName,
           g_ad_block_component_id_,
           g_ad_block_component_base64_public_key_);
  return true;
}

void AdBlockService::OnComponentReady(const std::string& component_id,
                                      const base::FilePath& install_dir,
                                      const std::string& manifest) {
  base::FilePath dat_file_path =
      install_dir.AppendASCII(DAT_FILE);
  GetDATFileData(dat_file_path);
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

}  // namespace brave_shields
