/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_PARSING_H_

#include <map>
#include <string>

#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom-forward.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom-forward.h"

namespace brave_ads {

using SearchResultAdMap =
    std::map<std::string, ads::mojom::SearchResultAdInfoPtr>;

SearchResultAdMap ParseWebPageEntities(blink::mojom::WebPagePtr web_page);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_PARSING_H_
