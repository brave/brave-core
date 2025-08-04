// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_pref_service.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
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

AdBlockPrefService::AdBlockPrefService(
    AdBlockService* ad_block_service,
    PrefService* prefs,
    PrefService* local_state,
    std::string locale,
    HostContentSettingsMap* host_content_settings_map)
    : ad_block_service_(ad_block_service),
      prefs_(prefs),
      host_content_settings_map_(host_content_settings_map),
      locale_(std::move(locale)) {
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

  pref_change_registrar_->Add(
      prefs::kAdBlockDeveloperMode,
      base::BindRepeating(&AdBlockPrefService::OnDeveloperModeChanged,
                          base::Unretained(this)));

  bool is_adblock_only_mode_enabled = false;
  if (IsAdblockOnlyModeSupported()) {
    is_adblock_only_mode_enabled =
        local_state &&
        local_state->GetBoolean(prefs::kAdBlockAdblockOnlyModeEnabled);

    pref_change_registrar_->Add(
        prefs::kAdBlockAdblockOnlyModeEnabled,
        base::BindRepeating(&AdBlockPrefService::OnAdBlockOnlyModeChanged,
                            base::Unretained(this)));

    CHECK(host_content_settings_map_);
    host_content_settings_map_->AddObserver(this);
  }
  // TODO(aseren): Currently we use both local state and content settings.
  // Consider moving to local state usage only.
  SetBraveShieldsAdBlockOnlyModeEnabled(host_content_settings_map_,
                                        is_adblock_only_mode_enabled, GURL(),
                                        local_state);

  OnDeveloperModeChanged();
  OnAdBlockOnlyModeChanged();
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

bool AdBlockPrefService::IsAdblockOnlyModeSupported() const {
  return IsAdblockOnlyModeFeatureEnabled() &&
         IsAdblockOnlyModeSupportedForLocale(locale_);
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

void AdBlockPrefService::OnDeveloperModeChanged() {
  const bool enabled = prefs_->GetBoolean(prefs::kAdBlockDeveloperMode);
  ad_block_service_->EnableDeveloperMode(enabled);
}

void AdBlockPrefService::OnAdBlockOnlyModeChanged() {
  const bool enabled =
      GetBraveShieldsAdBlockOnlyModeEnabled(host_content_settings_map_, GURL());
  ad_block_service_->EnableAdBlockOnlyMode(enabled);
}

void AdBlockPrefService::OnProxyConfigChanged(
    const net::ProxyConfigWithAnnotation& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  last_proxy_config_availability_ = availability;
  last_proxy_config_ = config;
}

void AdBlockPrefService::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsTypeSet content_type_set) {
  if (!content_type_set.Contains(
          ContentSettingsType::BRAVE_SHIELDS_AD_BLOCK_ONLY_MODE)) {
    return;
  }

  if (primary_pattern != ContentSettingsPattern::Wildcard() &&
      secondary_pattern != ContentSettingsPattern::Wildcard()) {
    return;
  }

  OnAdBlockOnlyModeChanged();
}

}  // namespace brave_shields
