/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_util_impl.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToBraveNewsAds() || UserHasOptedInToNotificationAds();
}

}  // namespace

PurchaseIntentResource::PurchaseIntentResource() {
  AddAdsClientNotifierObserver(this);
}

PurchaseIntentResource::~PurchaseIntentResource() {
  RemoveAdsClientNotifierObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void PurchaseIntentResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void PurchaseIntentResource::MaybeLoadOrReset() {
  DidLoad() ? MaybeReset() : MaybeLoad();
}

void PurchaseIntentResource::Load() {
  did_load_ = true;

  LoadAndParseResource(kPurchaseIntentResourceId,
                       kPurchaseIntentResourceVersion.Get(),
                       base::BindOnce(&PurchaseIntentResource::LoadCallback,
                                      weak_factory_.GetWeakPtr()));
}

void PurchaseIntentResource::LoadCallback(
    ResourceParsingErrorOr<PurchaseIntentInfo> result) {
  if (!result.has_value()) {
    return BLOG(0, "Failed to initialize " << kPurchaseIntentResourceId
                                           << " purchase intent resource ("
                                           << result.error() << ")");
  }

  if (result.value().version == 0) {
    return BLOG(1, kPurchaseIntentResourceId
                       << " purchase intent resource is not available");
  }

  BLOG(1, "Successfully loaded " << kPurchaseIntentResourceId
                                 << " purchase intent resource");

  purchase_intent_ = std::move(result).value();

  BLOG(1, "Successfully initialized " << kPurchaseIntentResourceId
                                      << " purchase intent resource version "
                                      << kPurchaseIntentResourceVersion.Get());
}

void PurchaseIntentResource::MaybeReset() {
  if (DidLoad() && !DoesRequireResource()) {
    Reset();
  }
}

void PurchaseIntentResource::Reset() {
  BLOG(1, "Reset " << kPurchaseIntentResourceId << " purchase intent resource");
  purchase_intent_.reset();
  did_load_ = false;
}

void PurchaseIntentResource::OnNotifyLocaleDidChange(
    const std::string& /*locale=*/) {
  MaybeLoad();
}

void PurchaseIntentResource::OnNotifyPrefDidChange(const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled ||
      path == prefs::kOptedInToNotificationAds ||
      path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday) {
    MaybeLoadOrReset();
  }
}

void PurchaseIntentResource::OnNotifyDidUpdateResourceComponent(
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

void PurchaseIntentResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidCountryComponentId(id)) {
    return;
  }

  manifest_version_.reset();

  Reset();
}

}  // namespace brave_ads
