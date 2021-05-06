/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_pref_service.h"

#include "base/bind.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

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

AdBlockPrefService::AdBlockPrefService(AdBlockService* ad_block_service,
                                       PrefService* prefs)
    : ad_block_service_(ad_block_service), prefs_(prefs) {
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
  ad_block_service_->EnableTag(tag, enabled);
  ad_block_service_->regional_service_manager()->EnableTag(tag, enabled);
  ad_block_service_->custom_filters_service()->EnableTag(tag, enabled);
}

}  // namespace brave_shields
