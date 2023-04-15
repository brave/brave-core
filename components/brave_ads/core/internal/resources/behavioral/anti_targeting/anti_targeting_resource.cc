/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "brave/components/brave_ads/core/internal/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads::resource {

namespace {
constexpr char kResourceId[] = "mkdhnfmjhklfnamlheoliekgeohamoig";
}  // namespace

AntiTargeting::AntiTargeting() {
  AdsClientHelper::AddObserver(this);
}

AntiTargeting::~AntiTargeting() {
  AdsClientHelper::RemoveObserver(this);
}

void AntiTargeting::Load() {
  LoadAndParseResource(kResourceId, features::GetAntiTargetingResourceVersion(),
                       base::BindOnce(&AntiTargeting::OnLoadAndParseResource,
                                      weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void AntiTargeting::OnLoadAndParseResource(
    ParsingErrorOr<AntiTargetingInfo> result) {
  if (!result.has_value()) {
    BLOG(1, result.error());
    BLOG(1,
         "Failed to initialize " << kResourceId << " anti-targeting resource");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " anti-targeting resource");
  anti_targeting_ = std::move(result).value();

  BLOG(1, "Parsed anti-targeting resource version " << anti_targeting_.version);

  is_initialized_ = true;

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " anti-targeting resource");
}

void AntiTargeting::OnNotifyLocaleDidChange(const std::string& /*locale*/) {
  Load();
}

void AntiTargeting::OnNotifyDidUpdateResourceComponent(const std::string& id) {
  if (IsValidCountryComponentId(id)) {
    Load();
  }
}

}  // namespace brave_ads::resource
