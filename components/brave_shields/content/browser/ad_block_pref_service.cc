// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_pref_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker.h"

namespace brave_shields {

namespace {

std::string GetTagFromPrefName(const std::string& pref_name) {
  if (pref_name == prefs::kFBEmbedControlType) {
    return brave_shields::kFacebookEmbeds;
  }
  if (pref_name == prefs::kTwitterEmbedControlType) {
    return brave_shields::kTwitterEmbeds;
  }
  if (pref_name == prefs::kLinkedInEmbedControlType) {
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
      prefs::kFBEmbedControlType,
      base::BindRepeating(&AdBlockPrefService::OnPreferenceChanged,
                          base::Unretained(this), prefs::kFBEmbedControlType));
  pref_change_registrar_->Add(
      prefs::kTwitterEmbedControlType,
      base::BindRepeating(&AdBlockPrefService::OnPreferenceChanged,
                          base::Unretained(this),
                          prefs::kTwitterEmbedControlType));
  pref_change_registrar_->Add(
      prefs::kLinkedInEmbedControlType,
      base::BindRepeating(&AdBlockPrefService::OnPreferenceChanged,
                          base::Unretained(this),
                          prefs::kLinkedInEmbedControlType));
  OnPreferenceChanged(prefs::kFBEmbedControlType);
  OnPreferenceChanged(prefs::kTwitterEmbedControlType);
  OnPreferenceChanged(prefs::kLinkedInEmbedControlType);
}

AdBlockPrefService::~AdBlockPrefService() = default;

void AdBlockPrefService::StartProxyTracker(
    std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker,
    std::unique_ptr<net::ProxyConfigService> proxy_config_service) {
  DCHECK(!pref_proxy_config_tracker_ && pref_proxy_config_tracker);
  DCHECK(!proxy_config_service_ && proxy_config_service);

  pref_proxy_config_tracker_ = std::move(pref_proxy_config_tracker);
  proxy_config_service_ = std::move(proxy_config_service);

  last_proxy_config_availability_ =
      proxy_config_service_->GetLatestProxyConfig(&last_proxy_config_);
  proxy_config_service_->AddObserver(this);
}

net::ProxyConfigService::ConfigAvailability
AdBlockPrefService::GetLatestProxyConfig(
    net::ProxyConfigWithAnnotation* config) const {
  DCHECK(pref_proxy_config_tracker_ && proxy_config_service_);

  *config = last_proxy_config_;
  return last_proxy_config_availability_;
}

void AdBlockPrefService::Shutdown() {
  pref_change_registrar_.reset();

  // `pref_proxy_config_tracker_` has a reference to `proxy_config_service_`,
  // therefore detach `pref_proxy_config_tracker_` first, to prevent the
  // reference from dangling.

  if (pref_proxy_config_tracker_) {
    pref_proxy_config_tracker_->DetachFromPrefService();
    pref_proxy_config_tracker_.reset();
  }

  if (proxy_config_service_) {
    proxy_config_service_->RemoveObserver(this);
    proxy_config_service_.reset();
  }
}

void AdBlockPrefService::OnPreferenceChanged(const std::string& pref_name) {
  std::string tag = GetTagFromPrefName(pref_name);
  if (tag.length() == 0) {
    return;
  }
  bool enabled = prefs_->GetBoolean(pref_name);
  ad_block_service_->EnableTag(tag, enabled);
}

void AdBlockPrefService::OnProxyConfigChanged(
    const net::ProxyConfigWithAnnotation& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  last_proxy_config_availability_ = availability;
  last_proxy_config_ = config;
}

}  // namespace brave_shields
