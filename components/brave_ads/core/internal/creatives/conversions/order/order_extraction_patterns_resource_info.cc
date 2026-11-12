/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource_info.h"

#include <string_view>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"

namespace brave_ads {

namespace {

constexpr std::string_view kVersionKey = "version";
constexpr std::string_view kConversionsKey = "conversions";
constexpr std::string_view kOrderKey = "order";

}  // namespace

OrderExtractionPatternsResourceInfo::OrderExtractionPatternsResourceInfo() =
    default;

OrderExtractionPatternsResourceInfo::OrderExtractionPatternsResourceInfo(
    OrderExtractionPatternsResourceInfo&&) noexcept = default;

OrderExtractionPatternsResourceInfo&
OrderExtractionPatternsResourceInfo::operator=(
    OrderExtractionPatternsResourceInfo&&) noexcept = default;

OrderExtractionPatternsResourceInfo::~OrderExtractionPatternsResourceInfo() =
    default;

// static
std::optional<OrderExtractionPatternsResourceInfo>
OrderExtractionPatternsResourceInfo::MaybeFromDict(const base::DictValue dict) {
  OrderExtractionPatternsResourceInfo resource;

  // An unsupported version is not malformed content, so it is left for the
  // caller to decide, mirroring `AntiTargetingResourceInfo::MaybeFromDict`.
  resource.version = dict.FindInt(kVersionKey);

  const base::DictValue* const conversions_dict =
      dict.FindDict(kConversionsKey);
  const base::DictValue* const order_dict =
      conversions_dict ? conversions_dict->FindDict(kOrderKey) : nullptr;
  if (order_dict) {
    resource.patterns = json::reader::ReadOrderPatterns(*order_dict);
  }

  return resource;
}

}  // namespace brave_ads
