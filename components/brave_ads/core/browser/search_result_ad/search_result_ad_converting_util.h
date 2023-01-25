/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_CONVERTING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_CONVERTING_UTIL_H_

#include <vector>

#include "base/containers/flat_map.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom-forward.h"
#include "components/schema_org/common/metadata.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

using SearchResultAdMap =
    base::flat_map<GURL, ads::mojom::SearchResultAdInfoPtr>;

SearchResultAdMap ConvertWebPageEntitiesToSearchResultAds(
    const std::vector<::schema_org::mojom::EntityPtr>& web_page_entities);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_CONVERTING_UTIL_H_
