/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/purchase_intent_classifier/funnel_keyword_info.h"

namespace ads {
namespace classification {

FunnelKeywordInfo::FunnelKeywordInfo() = default;

FunnelKeywordInfo::FunnelKeywordInfo(
    const std::string& keywords,
    const uint16_t weight)
    : keywords(keywords),
      weight(weight) {}

FunnelKeywordInfo::~FunnelKeywordInfo() = default;

}  // namespace classification
}  // namespace ads
