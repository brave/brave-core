/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_util_impl.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_constants.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  // Require resource only if:
  // - The user has joined Brave Rewards and opted into notification ads.
  return UserHasOptedInToNotificationAds();
}

}  // namespace

PurchaseIntentResource::PurchaseIntentResource() {
  GetAdsClient().AddObserver(this);
}

PurchaseIntentResource::~PurchaseIntentResource() {
  GetAdsClient().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void PurchaseIntentResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void PurchaseIntentResource::MaybeLoadOrUnload() {
  IsLoaded() ? MaybeUnload() : MaybeLoad();
}

void PurchaseIntentResource::Load() {
  LoadAndParseResourceComponent(
      kPurchaseIntentResourceId, kPurchaseIntentResourceVersion.Get(),
      base::BindOnce(&PurchaseIntentResource::LoadCallback,
                     weak_factory_.GetWeakPtr()));
}

void PurchaseIntentResource::LoadCallback(
    ResourceComponentParsingErrorOr<PurchaseIntentResourceInfo> result) {
  if (!result.has_value()) {
    return BLOG(0, "Failed to load and parse " << kPurchaseIntentResourceId
                                               << " purchase intent resource ("
                                               << result.error() << ")");
  }
  PurchaseIntentResourceInfo& resource = result.value();

  if (!resource.version) {
    return BLOG(1, kPurchaseIntentResourceId
                       << " purchase intent resource is unavailable");
  }

  resource_ = std::move(resource);

  BLOG(1, "Successfully loaded and parsed "
              << kPurchaseIntentResourceId
              << " purchase intent resource version "
              << kPurchaseIntentResourceVersion.Get());
}

void PurchaseIntentResource::MaybeUnload() {
  if (manifest_version_ && !DoesRequireResource()) {
    Unload();
  }
}

void PurchaseIntentResource::Unload() {
  BLOG(1,
       "Unloaded " << kPurchaseIntentResourceId << " purchase intent resource");

  resource_.reset();
}

void PurchaseIntentResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  MaybeLoad();
}

void PurchaseIntentResource::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    MaybeLoadOrUnload();
  }
}

void PurchaseIntentResource::OnNotifyResourceComponentDidChange(
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
    BLOG(1, "Registering "
                << id << " purchase intent resource component manifest version "
                << manifest_version);
  } else {
    BLOG(1,
         "Updating " << id
                     << " purchase intent resource component manifest version "
                     << *manifest_version_ << " to " << manifest_version);
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

void PurchaseIntentResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  BLOG(1, "Unregistering " << id << " purchase intent resource component");

  manifest_version_.reset();

  Unload();
}

}  // namespace brave_ads
