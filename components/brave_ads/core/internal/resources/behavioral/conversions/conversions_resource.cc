/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_info.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads::resource {

namespace {
constexpr char kResourceId[] = "nnqccijfhvzwyrxpxwjrpmynaiazctqb";
}  // namespace

Conversions::Conversions() = default;

Conversions::~Conversions() = default;

void Conversions::Load() {
  LoadAndParseResource(kResourceId, kConversionsResourceVersion.Get(),
                       base::BindOnce(&Conversions::OnLoadAndParseResource,
                                      weak_factory_.GetWeakPtr()));
}

void Conversions::OnLoadAndParseResource(
    ParsingErrorOr<ConversionsInfo> result) {
  if (!result.has_value()) {
    BLOG(1, result.error());
    BLOG(1, "Failed to initialize " << kResourceId << " conversions resource");
    is_initialized_ = false;
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " conversions resource");
  conversions_ = std::move(result).value();

  BLOG(1, "Parsed conversions resource version " << conversions_.version);

  is_initialized_ = true;

  BLOG(1,
       "Successfully initialized " << kResourceId << " conversions resource");
}

}  // namespace brave_ads::resource
