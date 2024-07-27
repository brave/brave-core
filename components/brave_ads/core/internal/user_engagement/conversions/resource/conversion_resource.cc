/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_util_impl.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_info.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  // Require resource only if:
  // - The user has joined Brave Rewards and opted into Brave News ads, new tab
  //   page ads, notification ads, or search result ads.
  return UserHasJoinedBraveRewards() &&
         (UserHasOptedInToBraveNewsAds() || UserHasOptedInToNewTabPageAds() ||
          UserHasOptedInToNotificationAds() ||
          UserHasOptedInToSearchResultAds());
}

}  // namespace

ConversionResource::ConversionResource() {
  AddAdsClientNotifierObserver(this);
}

ConversionResource::~ConversionResource() {
  RemoveAdsClientNotifierObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void ConversionResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void ConversionResource::MaybeLoadOrUnload() {
  IsLoaded() ? MaybeUnload() : MaybeLoad();
}

void ConversionResource::Load() {
  LoadAndParseResourceComponent(
      kConversionResourceId, kConversionResourceVersion.Get(),
      base::BindOnce(&ConversionResource::LoadCallback,
                     weak_factory_.GetWeakPtr()));
}

void ConversionResource::LoadCallback(
    ResourceComponentParsingErrorOr<ConversionResourceInfo> result) {
  if (!result.has_value()) {
    return BLOG(0, "Failed to load and parse " << kConversionResourceId
                                               << " conversion resource ("
                                               << result.error() << ")");
  }
  ConversionResourceInfo& resource = result.value();

  if (!resource.version) {
    return BLOG(1,
                kConversionResourceId << " conversion resource is unavailable");
  }

  resource_ = std::move(resource);

  BLOG(1, "Successfully loaded and parsed "
              << kConversionResourceId << " conversion resource version "
              << kConversionResourceVersion.Get());
}

void ConversionResource::MaybeUnload() {
  if (manifest_version_ && !DoesRequireResource()) {
    Unload();
  }
}

void ConversionResource::Unload() {
  BLOG(1, "Unloaded " << kConversionResourceId << " conversion resource");

  resource_.reset();
}

void ConversionResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  MaybeLoad();
}

void ConversionResource::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToSearchResultAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    MaybeLoadOrUnload();
  }
}

void ConversionResource::OnNotifyResourceComponentDidChange(
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
    BLOG(1, "Registering " << id
                           << " conversion resource component manifest version "
                           << manifest_version);
  } else {
    BLOG(1, "Updating " << id
                        << " conversion resource component manifest version "
                        << *manifest_version_ << " to " << manifest_version);
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

void ConversionResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  BLOG(1, "Unregistering " << id << " conversion resource component");

  manifest_version_.reset();

  Unload();
}

}  // namespace brave_ads
