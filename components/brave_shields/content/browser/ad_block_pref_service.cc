// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_pref_service.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_shields/core/browser/brave_shields_locale_utils.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker.h"
#include "components/user_prefs/user_prefs.h"

namespace {

inline constexpr char kLegacyFBEmbedControlType[] = "brave.fb_embed_default";
inline constexpr char kLegacyTwitterEmbedControlType[] =
    "brave.twitter_embed_default";
inline constexpr char kLegacyLinkedInEmbedControlType[] =
    "brave.linkedin_embed_default";
}

namespace brave_shields {

AdBlockPrefService::AdBlockPrefService(bool is_regular_profile,
                                       PrefService* prefs,
                                       PrefService* local_state,
                                       const std::string& locale) {
  if (is_regular_profile &&
      !local_state->GetBoolean(
          prefs::kMigratedAdblockSocialMediaBlockingSettings)) {
    // Yes, we're reading these on the first profile launched, and migrating
    // them to Local State. It's not ideal, but that's how the settings have
    // existed for a long time.
    for (const auto& [old_pref, new_pref] :
         {std::pair{kLegacyFBEmbedControlType, prefs::kFBEmbedControlType},
          std::pair{kLegacyTwitterEmbedControlType,
                    prefs::kTwitterEmbedControlType},
          std::pair{kLegacyLinkedInEmbedControlType,
                    prefs::kLinkedInEmbedControlType}}) {
      if (prefs->FindPreference(old_pref)->HasUserSetting()) {
        local_state->SetBoolean(new_pref, prefs->GetBoolean(old_pref));
      }
    }
    local_state->SetBoolean(prefs::kMigratedAdblockSocialMediaBlockingSettings,
                            true);
  }

  ManageAdBlockOnlyModeByLocale(local_state, locale);
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

void AdBlockPrefService::OnProxyConfigChanged(
    const net::ProxyConfigWithAnnotation& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  last_proxy_config_availability_ = availability;
  last_proxy_config_ = config;
}

// static
void AdBlockPrefService::RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kLegacyFBEmbedControlType, true);
  registry->RegisterBooleanPref(kLegacyTwitterEmbedControlType, true);
  registry->RegisterBooleanPref(kLegacyLinkedInEmbedControlType, false);
}

}  // namespace brave_shields
