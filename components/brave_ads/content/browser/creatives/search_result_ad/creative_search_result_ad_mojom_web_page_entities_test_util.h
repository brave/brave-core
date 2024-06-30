/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_WEB_PAGE_ENTITIES_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_WEB_PAGE_ENTITIES_TEST_UTIL_H_

#include <string_view>
#include <vector>

#include "components/schema_org/common/metadata.mojom-forward.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom-forward.h"

namespace brave_ads::test {

std::vector<schema_org::mojom::EntityPtr>
CreativeSearchResultAdMojomWebPageEntities(
    std::vector<std::string_view> excluded_property_names);

blink::mojom::WebPagePtr CreativeSearchResultAdMojomWebPage(
    std::vector<std::string_view> excluded_property_names);

std::vector<schema_org::mojom::EntityPtr>
CreativeSearchResultAdMojomWebPageEntitiesWithProperty(std::string_view name,
                                                       std::string_view value);

blink::mojom::WebPagePtr CreativeSearchResultAdMojomWebPageWithProperty(
    std::string_view name,
    std::string_view value);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_WEB_PAGE_ENTITIES_TEST_UTIL_H_
