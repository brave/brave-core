/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"

#include <utility>
#include <vector>

#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_info.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/resources_util_impl.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace resource {

namespace {
constexpr char kResourceId[] = "bejenkminijgplakmkmcgkhjjnkelbld";
}  // namespace

PurchaseIntent::PurchaseIntent()
    : purchase_intent_(std::make_unique<ad_targeting::PurchaseIntentInfo>()) {}

PurchaseIntent::~PurchaseIntent() = default;

bool PurchaseIntent::IsInitialized() const {
  return is_initialized_;
}

void PurchaseIntent::Load() {
  LoadAndParseResource(kResourceId,
                       features::GetPurchaseIntentResourceVersion(),
                       base::BindOnce(&PurchaseIntent::OnLoadAndParseResource,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void PurchaseIntent::OnLoadAndParseResource(
    ParsingResultPtr<ad_targeting::PurchaseIntentInfo> result) {
  if (!result) {
    BLOG(1, "Failed to load " << kResourceId << " purchase intent resource");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " purchase intent resource");

  if (!result->resource) {
    BLOG(1, result->error_message);
    BLOG(1,
         "Failed to initialize " << kResourceId << " purchase intent resource");
    is_initialized_ = false;
    return;
  }

  purchase_intent_ = std::move(result->resource);

  BLOG(1,
       "Parsed purchase intent resource version " << purchase_intent_->version);

  is_initialized_ = true;

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " purchase intent resource");
}

const ad_targeting::PurchaseIntentInfo* PurchaseIntent::get() const {
  return purchase_intent_.get();
}

}  // namespace resource
}  // namespace ads
