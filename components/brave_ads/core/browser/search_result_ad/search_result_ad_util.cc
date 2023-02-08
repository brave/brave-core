/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_util.h"

#include "base/strings/string_piece.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

namespace brave_ads {

namespace {

constexpr char kSearchResultAdClickedPath[] = "/a/redirect";
constexpr char kPlacementIdParameterName[] = "placement_id";

}  // namespace

absl::optional<std::string> GetPlacementIdFromSearchResultAdClickedUrl(
    const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme) ||
      url.path_piece() != kSearchResultAdClickedPath || !url.has_query() ||
      !brave_search::IsAllowedHost(url)) {
    return {};
  }

  base::StringPiece query_str = url.query_piece();
  url::Component query(0, static_cast<int>(query_str.length()));
  url::Component key;
  url::Component value;
  while (url::ExtractQueryKeyValue(query_str.data(), &query, &key, &value)) {
    base::StringPiece key_str = query_str.substr(key.begin, key.len);
    if (key_str == kPlacementIdParameterName) {
      base::StringPiece value_str = query_str.substr(value.begin, value.len);
      return static_cast<std::string>(value_str);
    }
  }

  return {};
}

}  // namespace brave_ads
