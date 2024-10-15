/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_WEB_PAGE_ENTITIES_EXTRACTOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_WEB_PAGE_ENTITIES_EXTRACTOR_H_

#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "components/schema_org/common/metadata.mojom-forward.h"

namespace brave_ads {

std::vector<mojom::CreativeSearchResultAdInfoPtr>
ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
    const std::vector<schema_org::mojom::EntityPtr>& mojom_entities);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_WEB_PAGE_ENTITIES_EXTRACTOR_H_
