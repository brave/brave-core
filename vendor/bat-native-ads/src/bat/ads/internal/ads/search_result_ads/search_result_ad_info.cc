/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ads/search_result_ad_info.h"

namespace ads {

SearchResultAdInfo::SearchResultAdInfo() = default;

SearchResultAdInfo::SearchResultAdInfo(const SearchResultAdInfo& info) =
    default;

SearchResultAdInfo::~SearchResultAdInfo() = default;

bool SearchResultAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (headline_text.empty() || description.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
