/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_info.h"
#include "brave/components/brave_ads/core/internal/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "nnqccijfhvzwyrxpxwjrpmynaiazctqb";
}  // namespace

ConversionsResource::ConversionsResource() {
  AdsClientHelper::AddObserver(this);
}

ConversionsResource::~ConversionsResource() {
  AdsClientHelper::RemoveObserver(this);
}

void ConversionsResource::Load() {
  LoadAndParseResource(
      kResourceId, kConversionsResourceVersion.Get(),
      base::BindOnce(&ConversionsResource::LoadAndParseResourceCallback,
                     weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void ConversionsResource::LoadAndParseResourceCallback(
    ResourceParsingErrorOr<ConversionsInfo> result) {
  if (!result.has_value()) {
    BLOG(0, "Failed to initialize " << kResourceId << " conversions resource ("
                                    << result.error() << ")");
    is_initialized_ = false;
    return;
  }

  if (result.value().version == 0) {
    BLOG(7, kResourceId << " conversions resource does not exist");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " conversions resource");

  conversions_ = std::move(result).value();

  is_initialized_ = true;

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " conversions resource version "
                                      << kConversionsResourceVersion.Get());
}

void ConversionsResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  Load();
}

void ConversionsResource::OnNotifyDidUpdateResourceComponent(
    const std::string& id) {
  if (IsValidCountryComponentId(id)) {
    Load();
  }
}

}  // namespace brave_ads
