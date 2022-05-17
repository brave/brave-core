/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include <utility>

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"
#include "bat/ads/internal/resources/resources_util_impl.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace resource {

namespace {
constexpr char kResourceId[] = "mkdhnfmjhklfnamlheoliekgeohamoig";
}  // namespace

AntiTargeting::AntiTargeting()
    : anti_targeting_(std::make_unique<AntiTargetingInfo>()) {}

AntiTargeting::~AntiTargeting() = default;

bool AntiTargeting::IsInitialized() const {
  return is_initialized_;
}

void AntiTargeting::Load() {
  LoadAndParseResource(kResourceId, features::GetAntiTargetingResourceVersion(),
                       base::BindOnce(&AntiTargeting::OnLoadAndParseResource,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void AntiTargeting::OnLoadAndParseResource(
    ParsingResultPtr<AntiTargetingInfo> result) {
  if (!result) {
    BLOG(1, "Failed to load resource " << kResourceId);
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded resource " << kResourceId);

  if (!result->resource) {
    BLOG(1, result->error_message);
    BLOG(1, "Failed to initialize resource " << kResourceId);
    is_initialized_ = false;
    return;
  }

  anti_targeting_ = std::move(result->resource);

  BLOG(1,
       "Parsed anti targeting resource version " << anti_targeting_->version);

  is_initialized_ = true;

  BLOG(1, "Successfully initialized resource " << kResourceId);
}

AntiTargetingInfo AntiTargeting::get() const {
  return *anti_targeting_;
}

}  // namespace resource
}  // namespace ads
