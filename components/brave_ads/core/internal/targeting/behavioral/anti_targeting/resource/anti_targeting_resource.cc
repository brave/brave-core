/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_util_impl.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_constants.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_info.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

namespace {

bool DoesRequireResourceForNewTabPageAds() {
  // Require resource only if:
  // - The user has opted into new tab page ads and has either joined Brave
  //   Rewards or new tab page ad events should always be triggered.
  return UserHasOptedInToNewTabPageAds() &&
         (UserHasJoinedBraveRewards() ||
          ShouldAlwaysTriggerNewTabPageAdEvents());
}

bool DoesRequireResource() {
  // Require resource only if:
  // - The user has opted into Brave News ads.
  // - The user has opted into new tab page ads and has either joined Brave
  //   Rewards or new tab page ad events should always be triggered.
  // - The user has joined Brave Rewards and opted into notification ads.
  return UserHasOptedInToBraveNewsAds() ||
         DoesRequireResourceForNewTabPageAds() ||
         UserHasOptedInToNotificationAds();
}

}  // namespace

AntiTargetingResource::AntiTargetingResource() {
  GetAdsClient().AddObserver(this);
}

AntiTargetingResource::~AntiTargetingResource() {
  GetAdsClient().RemoveObserver(this);
}

AntiTargetingSiteList AntiTargetingResource::GetSites(
    const std::string& creative_set_id) const {
  if (!resource_) {
    return {};
  }

  const auto iter = resource_->creative_sets.find(creative_set_id);
  if (iter == resource_->creative_sets.cend()) {
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

void AntiTargetingResource::MaybeLoadOrUnload() {
  IsLoaded() ? MaybeUnload() : MaybeLoad();
}

void AntiTargetingResource::Load() {
  LoadAndParseResourceComponent(
      kAntiTargetingResourceId, kAntiTargetingResourceVersion.Get(),
      base::BindOnce(&AntiTargetingResource::LoadCallback,
                     weak_factory_.GetWeakPtr()));
}

void AntiTargetingResource::LoadCallback(
    ResourceComponentParsingErrorOr<AntiTargetingResourceInfo> result) {
  if (!result.has_value()) {
    return BLOG(0, "Failed to load and parse " << kAntiTargetingResourceId
                                               << " anti-targeting resource ("
                                               << result.error() << ")");
  }
  AntiTargetingResourceInfo& resource = result.value();

  if (!resource.version) {
    return BLOG(1, kAntiTargetingResourceId
                       << " anti-targeting resource is unavailable");
  }

  resource_ = std::move(resource);

  BLOG(1, "Successfully loaded and parsed "
              << kAntiTargetingResourceId << " anti-targeting resource version "
              << kAntiTargetingResourceVersion.Get());
}

void AntiTargetingResource::MaybeUnload() {
  if (!DoesRequireResource()) {
    Unload();
  }
}

void AntiTargetingResource::Unload() {
  BLOG(1,
       "Unloaded " << kAntiTargetingResourceId << " anti-targeting resource");

  resource_.reset();
}

void AntiTargetingResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  MaybeLoad();
}

void AntiTargetingResource::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    MaybeLoadOrUnload();
  }
}

void AntiTargetingResource::OnNotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  if (manifest_version == manifest_version_) {
    // No need to load the resource if the manifest version is the same.
    return;
  }

  if (!manifest_version_) {
    BLOG(
        1,
        "Registering " << id
                       << " anti-targeting resource component manifest version "
                       << manifest_version);
  } else {
    BLOG(1,
         "Updating " << id
                     << " anti-targeting resource component manifest version "
                     << *manifest_version_ << " to " << manifest_version);
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

void AntiTargetingResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  BLOG(1, "Unregistering " << id << " anti-targeting resource component");

  manifest_version_.reset();

  Unload();
}

}  // namespace brave_ads
