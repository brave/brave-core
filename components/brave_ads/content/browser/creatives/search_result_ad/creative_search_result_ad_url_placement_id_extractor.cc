/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_url_placement_id_extractor.h"

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_url_util.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr std::string_view kPlacementIdQueryKey = "placement_id";

}  // namespace

std::optional<std::string> MaybeExtractCreativeAdPlacementIdFromUrl(
    const GURL& url) {
  if (!IsSearchResultAdRedirectUrl(url) || !url.has_query()) {
    return std::nullopt;
  }

  for (net::QueryIterator iter(url); !iter.IsAtEnd(); iter.Advance()) {
    if (iter.GetKey() == kPlacementIdQueryKey) {
      std::string placement_id{iter.GetValue()};
      if (placement_id.empty()) {
        return std::nullopt;
      }

      return placement_id;
    }
  }

  return std::nullopt;
}

}  // namespace brave_ads
