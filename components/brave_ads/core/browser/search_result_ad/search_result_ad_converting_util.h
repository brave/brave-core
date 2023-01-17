/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_CONVERTING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_CONVERTING_UTIL_H_

#include "base/containers/flat_map.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom-forward.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom-forward.h"
#include "url/gurl.h"

namespace brave_ads {

using SearchResultAdMap =
    base::flat_map<GURL, ads::mojom::SearchResultAdInfoPtr>;

SearchResultAdMap ConvertWebPageToSearchResultAds(
    blink::mojom::WebPagePtr web_page);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_CONVERTING_UTIL_H_
