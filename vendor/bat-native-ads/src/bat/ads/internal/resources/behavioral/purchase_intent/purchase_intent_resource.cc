/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"
#include "bat/ads/internal/resources/resources_util_impl.h"

namespace ads::resource {

namespace {
constexpr char kResourceId[] = "bejenkminijgplakmkmcgkhjjnkelbld";
}  // namespace

PurchaseIntent::PurchaseIntent()
    : purchase_intent_(std::make_unique<targeting::PurchaseIntentInfo>()) {}

PurchaseIntent::~PurchaseIntent() = default;

bool PurchaseIntent::IsInitialized() const {
  return is_initialized_;
}

void PurchaseIntent::Load() {
  LoadAndParseResource(kResourceId,
                       targeting::features::GetPurchaseIntentResourceVersion(),
                       base::BindOnce(&PurchaseIntent::OnLoadAndParseResource,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void PurchaseIntent::OnLoadAndParseResource(
    ParsingResultPtr<targeting::PurchaseIntentInfo> result) {
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

const targeting::PurchaseIntentInfo* PurchaseIntent::Get() const {
  return purchase_intent_.get();
}

}  // namespace ads::resource
