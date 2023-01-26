/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "bat/ads/internal/resources/country_components.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/resources/resources_util_impl.h"

namespace ads::resource {

namespace {
constexpr char kResourceId[] = "mkdhnfmjhklfnamlheoliekgeohamoig";
}  // namespace

AntiTargeting::AntiTargeting()
    : anti_targeting_(std::make_unique<AntiTargetingInfo>()) {
  LocaleManager::GetInstance()->AddObserver(this);
  ResourceManager::GetInstance()->AddObserver(this);
}

AntiTargeting::~AntiTargeting() {
  LocaleManager::GetInstance()->RemoveObserver(this);
  ResourceManager::GetInstance()->RemoveObserver(this);
}

bool AntiTargeting::IsInitialized() const {
  return is_initialized_;
}

void AntiTargeting::Load() {
  LoadAndParseResource(kResourceId, features::GetAntiTargetingResourceVersion(),
                       base::BindOnce(&AntiTargeting::OnLoadAndParseResource,
                                      weak_ptr_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void AntiTargeting::OnLoadAndParseResource(
    ParsingResultPtr<AntiTargetingInfo> result) {
  if (!result) {
    BLOG(1, "Failed to load " << kResourceId << " anti-targeting resource");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " anti-targeting resource");

  if (!result->resource) {
    BLOG(1, result->error_message);
    BLOG(1,
         "Failed to initialize " << kResourceId << " anti-targeting resource");
    is_initialized_ = false;
    return;
  }

  anti_targeting_ = std::move(result->resource);

  BLOG(1,
       "Parsed anti-targeting resource version " << anti_targeting_->version);

  is_initialized_ = true;

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " anti-targeting resource");
}

void AntiTargeting::OnLocaleDidChange(const std::string& /*locale*/) {
  Load();
}

void AntiTargeting::OnResourceDidUpdate(const std::string& id) {
  if (IsValidCountryComponentId(id)) {
    Load();
  }
}

}  // namespace ads::resource
