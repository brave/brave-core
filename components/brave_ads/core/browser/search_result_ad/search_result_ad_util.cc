/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_util.h"

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr auto kSearchResultAdsConfirmationVettedHosts =
    base::MakeFixedFlatSet<base::StringPiece>(
        {"search.anonymous.ads.brave.com",
         "search.anonymous.ads.bravesoftware.com"});
constexpr char kSearchResultAdsClickedPath[] = "/v3/click";
constexpr char kCreativeInstanceIdParameterName[] = "creativeInstanceId";

bool IsSearchResultAdConfirmationUrl(const GURL& url, base::StringPiece path) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme) ||
      url.path_piece() != path) {
    return false;
  }

  if (!base::Contains(kSearchResultAdsConfirmationVettedHosts,
                      url.host_piece())) {
    return false;
  }

  return true;
}

}  // namespace

bool IsSearchResultAdClickedConfirmationUrl(const GURL& url) {
  return IsSearchResultAdConfirmationUrl(url, kSearchResultAdsClickedPath);
}

std::string GetClickedSearchResultAdCreativeInstanceId(const GURL& url) {
  if (!IsSearchResultAdClickedConfirmationUrl(url) || !url.has_query()) {
    return {};
  }

  base::StringPiece query_str = url.query_piece();
  url::Component query(0, static_cast<int>(query_str.length())), key, value;
  while (url::ExtractQueryKeyValue(query_str.data(), &query, &key, &value)) {
    base::StringPiece key_str = query_str.substr(key.begin, key.len);
    if (key_str == kCreativeInstanceIdParameterName) {
      base::StringPiece value_str = query_str.substr(value.begin, value.len);
      return static_cast<std::string>(value_str);
    }
  }

  return {};
}

}  // namespace brave_ads
