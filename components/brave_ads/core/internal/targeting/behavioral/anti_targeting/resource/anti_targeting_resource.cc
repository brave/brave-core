/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_util_impl.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToBraveNewsAds() || UserHasOptedInToNotificationAds();
}

}  // namespace

AntiTargetingResource::AntiTargetingResource() {
  AddAdsClientNotifierObserver(this);
}

AntiTargetingResource::~AntiTargetingResource() {
  RemoveAdsClientNotifierObserver(this);
}

AntiTargetingSiteList AntiTargetingResource::GetSites(
    const std::string& creative_set_id) const {
  if (!anti_targeting_) {
    return {};
  }

  const auto iter = anti_targeting_->creative_sets.find(creative_set_id);
  if (iter == anti_targeting_->creative_sets.cend()) {
    return {};
  }

  const auto& [_, sites] = *iter;

  return sites;
}

///////////////////////////////////////////////////////////////////////////////

void AntiTargetingResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void AntiTargetingResource::MaybeLoadOrReset() {
  DidLoad() ? MaybeReset() : MaybeLoad();
}

void AntiTargetingResource::Load() {
  did_load_ = true;

  LoadAndParseResource(kAntiTargetingResourceId,
                       kAntiTargetingResourceVersion.Get(),
                       base::BindOnce(&AntiTargetingResource::LoadCallback,
                                      weak_factory_.GetWeakPtr()));
}

void AntiTargetingResource::LoadCallback(
    ResourceParsingErrorOr<AntiTargetingInfo> result) {
  if (!result.has_value()) {
    return BLOG(0, "Failed to initialize " << kAntiTargetingResourceId
                                           << " anti-targeting resource ("
                                           << result.error() << ")");
  }

  if (result.value().version == 0) {
    return BLOG(1, kAntiTargetingResourceId
                       << " anti-targeting resource is not available");
  }

  BLOG(1, "Successfully loaded " << kAntiTargetingResourceId
                                 << " anti-targeting resource");

  anti_targeting_ = std::move(result).value();

  BLOG(1, "Successfully initialized " << kAntiTargetingResourceId
                                      << " anti-targeting resource version "
                                      << kAntiTargetingResourceVersion.Get());
}

void AntiTargetingResource::MaybeReset() {
  if (DidLoad() && !DoesRequireResource()) {
    Reset();
  }
}

void AntiTargetingResource::Reset() {
  BLOG(1, "Reset anti-targeting resource");
  anti_targeting_.reset();
  did_load_ = false;
}

void AntiTargetingResource::OnNotifyLocaleDidChange(
    const std::string& /*locale=*/) {
  MaybeLoad();
}

void AntiTargetingResource::OnNotifyPrefDidChange(const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled ||
      path == prefs::kOptedInToNotificationAds ||
      path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday) {
    MaybeLoadOrReset();
  }
}

void AntiTargetingResource::OnNotifyDidUpdateResourceComponent(
    const std::string& manifest_version,
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  if (manifest_version == manifest_version_) {
    return;
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

void AntiTargetingResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  manifest_version_.reset();

  Reset();
}

}  // namespace brave_ads
