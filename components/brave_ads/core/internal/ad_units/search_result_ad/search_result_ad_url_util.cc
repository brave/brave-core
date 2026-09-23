/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_url_util.h"

#include <string_view>

#include "brave/components/brave_search/common/brave_search_utils.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_ads {

namespace {
constexpr std::string_view kClickRedirectUrlPath = "/a/redirect";
}  // namespace

bool IsSearchResultAdRedirectUrl(const GURL& url) {
  return url.is_valid() && url.SchemeIs(url::kHttpsScheme) &&
         url.path() == kClickRedirectUrlPath &&
         brave_search::IsAllowedHost(url);
}

}  // namespace brave_ads
