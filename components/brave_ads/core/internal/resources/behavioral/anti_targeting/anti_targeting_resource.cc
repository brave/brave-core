/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_feature.h"
#include "brave/components/brave_ads/core/internal/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "mkdhnfmjhklfnamlheoliekgeohamoig";
}  // namespace

AntiTargetingResource::AntiTargetingResource() {
  AdsClientHelper::AddObserver(this);
}

AntiTargetingResource::~AntiTargetingResource() {
  AdsClientHelper::RemoveObserver(this);
}

void AntiTargetingResource::Load() {
  LoadAndParseResource(
      kResourceId, kAntiTargetingResourceVersion.Get(),
      base::BindOnce(&AntiTargetingResource::LoadAndParseResourceCallback,
                     weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void AntiTargetingResource::LoadAndParseResourceCallback(
    ResourceParsingErrorOr<AntiTargetingInfo> result) {
  if (!result.has_value()) {
    BLOG(0, "Failed to initialize "
                << kResourceId << " anti-targeting resource (" << result.error()
                << ")");
    is_initialized_ = false;
    return;
  }

  if (result.value().version == 0) {
    BLOG(7, kResourceId << " anti-targeting resource does not exist");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " anti-targeting resource");

  anti_targeting_ = std::move(result).value();

  is_initialized_ = true;

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " anti-targeting resource version "
                                      << kAntiTargetingResourceVersion.Get());
}

void AntiTargetingResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  Load();
}

void AntiTargetingResource::OnNotifyDidUpdateResourceComponent(
    const std::string& id) {
  if (IsValidCountryComponentId(id)) {
    Load();
  }
}

}  // namespace brave_ads
