/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "bejenkminijgplakmkmcgkhjjnkelbld";
}  // namespace

PurchaseIntentResource::PurchaseIntentResource() = default;

PurchaseIntentResource::~PurchaseIntentResource() = default;

void PurchaseIntentResource::Load() {
  LoadAndParseResource(
      kResourceId, kPurchaseIntentResourceVersion.Get(),
      base::BindOnce(&PurchaseIntentResource::OnLoadAndParseResource,
                     weak_factory_.GetWeakPtr()));
}

void PurchaseIntentResource::OnLoadAndParseResource(
    ResourceParsingErrorOr<PurchaseIntentInfo> result) {
  if (!result.has_value()) {
    BLOG(1, result.error());
    BLOG(1,
         "Failed to initialize " << kResourceId << " purchase intent resource");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " purchase intent resource");
  purchase_intent_ = std::move(result).value();

  BLOG(1,
       "Parsed purchase intent resource version " << purchase_intent_.version);

  is_initialized_ = true;

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " purchase intent resource");
}

const PurchaseIntentInfo* PurchaseIntentResource::Get() const {
  return &purchase_intent_;
}

}  // namespace brave_ads
