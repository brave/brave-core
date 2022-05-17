/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/conversions/conversions_resource.h"

#include <utility>

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/conversions/conversions_features.h"
#include "bat/ads/internal/resources/resources_util_impl.h"

namespace ads {
namespace resource {

namespace {
constexpr char kResourceId[] = "nnqccijfhvzwyrxpxwjrpmynaiazctqb";
}  // namespace

Conversions::Conversions()
    : conversions_info_(std::make_unique<ConversionsInfo>()) {}

Conversions::~Conversions() = default;

bool Conversions::IsInitialized() const {
  return is_initialized_;
}

void Conversions::Load() {
  LoadAndParseResource(kResourceId, features::GetConversionsResourceVersion(),
                       base::BindOnce(&Conversions::OnLoadAndParseResource,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void Conversions::OnLoadAndParseResource(
    ParsingResultPtr<ConversionsInfo> result) {
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

  conversions_info_ = std::move(result->resource);

  BLOG(1, "Parsed verifiable conversion resource version "
              << conversions_info_->version);

  is_initialized_ = true;

  BLOG(1, "Successfully initialized resource " << kResourceId);
}

const ConversionsInfo* Conversions::get() const {
  return conversions_info_.get();
}

}  // namespace resource
}  // namespace ads
