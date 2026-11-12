/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_util_impl.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_default_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_feature.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource_constants.h"

namespace brave_ads {

namespace {

// Compiled-in fallback, used until the component delivers a real set, or if
// it fails to load; mirrors Chromium's `AmountExtractionHeuristicRegexes`.
const OrderExtractionPatternsInfo& GetDefaultPatterns() {
  static const base::NoDestructor<OrderExtractionPatternsInfo> kPatterns([] {
    OrderExtractionPatternsInfo info;

    info.value.keyword_regexes["de"] =
        std::string(kDefaultOrderValueKeywordRegexDe);
    info.value.keyword_regexes["en"] =
        std::string(kDefaultOrderValueKeywordRegexEn);
    info.value.keyword_regexes["fr"] =
        std::string(kDefaultOrderValueKeywordRegexFr);
    info.value.currency_regexes["EUR"] =
        std::string(kDefaultOrderValueCurrencyRegexEur);
    info.value.currency_regexes["USD"] =
        std::string(kDefaultOrderValueCurrencyRegexUsd);
    info.value.currency_regexes["GBP"] =
        std::string(kDefaultOrderValueCurrencyRegexGbp);
    info.value.number_of_ancestor_levels_to_search =
        kDefaultNumberOfAncestorLevelsToSearch;

    info.id.source = "elementText";
    info.id.regexes["de"] = std::string(kDefaultOrderIdRegexDe);
    info.id.regexes["en"] = std::string(kDefaultOrderIdRegexEn);
    info.id.regexes["fr"] = std::string(kDefaultOrderIdRegexFr);

    return info;
  }());
  return *kPatterns;
}

}  // namespace

OrderExtractionPatternsResource::OrderExtractionPatternsResource() {
  ads_client_observation_.Observe(&GetAdsClient());

  Load();
}

OrderExtractionPatternsResource::~OrderExtractionPatternsResource() = default;

const OrderExtractionPatternsInfo*
OrderExtractionPatternsResource::GetPatterns() const {
  return resource_ ? &resource_->patterns : &GetDefaultPatterns();
}

///////////////////////////////////////////////////////////////////////////////

void OrderExtractionPatternsResource::Load() {
  LoadAndParseResourceComponent(
      kOrderExtractionPatternsResourceId,
      kOrderExtractionPatternsResourceVersion.Get(),
      base::BindOnce(&OrderExtractionPatternsResource::LoadCallback,
                     weak_factory_.GetWeakPtr()));
}

void OrderExtractionPatternsResource::LoadCallback(
    std::optional<OrderExtractionPatternsResourceInfo> resource,
    bool exists) {
  if (!exists) {
    resource_.reset();
    load_state_type_ = ResourceLoadStateType::kNotLoaded;
    return BLOG(1, kOrderExtractionPatternsResourceId
                       << " conversion order patterns resource is "
                          "unavailable");
  }

  if (!resource) {
    resource_.reset();
    load_state_type_ = ResourceLoadStateType::kFailedToLoad;
    return BLOG(0, "Failed to load and parse "
                       << kOrderExtractionPatternsResourceId
                       << " conversion order patterns resource");
  }

  if (!resource->version ||
      *resource->version != kOrderExtractionPatternsResourceVersion.Get()) {
    resource_.reset();
    load_state_type_ = ResourceLoadStateType::kNotLoaded;
    return BLOG(1, kOrderExtractionPatternsResourceId
                       << " conversion order patterns resource is "
                          "unavailable");
  }

  load_state_type_ = ResourceLoadStateType::kLoaded;
  resource_ = std::move(resource);

  BLOG(1, "Successfully loaded and parsed "
              << kOrderExtractionPatternsResourceId
              << " conversion order patterns resource version "
              << kOrderExtractionPatternsResourceVersion.Get());
}

void OrderExtractionPatternsResource::Unload() {
  BLOG(1, "Unloaded " << kOrderExtractionPatternsResourceId
                      << " conversion order patterns resource");

  resource_.reset();
  load_state_type_ = ResourceLoadStateType::kNotLoaded;
}

void OrderExtractionPatternsResource::OnNotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  if (manifest_version == manifest_version_) {
    // No need to load the resource if the manifest version is the same.
    return;
  }

  if (!manifest_version_) {
    BLOG(1, "Registering " << id
                           << " conversion order patterns resource "
                              "component manifest version "
                           << manifest_version);
  } else {
    BLOG(1, "Updating " << id
                        << " conversion order patterns resource component "
                           "manifest version "
                        << *manifest_version_ << " to " << manifest_version);
  }

  manifest_version_ = manifest_version;

  Load();
}

void OrderExtractionPatternsResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  BLOG(1, "Unregistering " << id
                           << " conversion order patterns resource "
                              "component");

  manifest_version_.reset();

  Unload();
}

}  // namespace brave_ads
