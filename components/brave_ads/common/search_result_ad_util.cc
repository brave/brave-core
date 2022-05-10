/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/search_result_ad_util.h"

#include "base/strings/string_piece.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

namespace brave_ads {

namespace {

constexpr char kSearchAdsConfirmationHost[] =
    "search-ads-confirmation.brave.com";
constexpr char kSearchAdsViewedPath[] = "/v10/view";
constexpr char kCreativeInstanceIdParameterName[] = "creativeInstanceId";

std::string GetCreativeInstanceIdFromUrl(const GURL& url,
                                         base::StringPiece host,
                                         base::StringPiece path) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme) ||
      url.host_piece() != host || url.path_piece() != path ||
      !url.has_query()) {
    return std::string();
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

  return std::string();
}

}  // namespace

std::string GetCreativeInstanceIdFromSearchAdsViewedUrl(const GURL& url) {
  return GetCreativeInstanceIdFromUrl(url, kSearchAdsConfirmationHost,
                                      kSearchAdsViewedPath);
}

}  // namespace brave_ads
