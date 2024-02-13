/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_AD_UNITS_SEARCH_RESULT_AD_TEST_WEB_PAGE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_AD_UNITS_SEARCH_RESULT_AD_TEST_WEB_PAGE_UTIL_H_

#include <string_view>
#include <vector>

#include "components/schema_org/common/metadata.mojom-forward.h"

namespace brave_ads {

inline constexpr char kTestWebPagePlacementId[] = "placement-id";

std::vector<::schema_org::mojom::EntityPtr> CreateTestWebPageEntities(
    std::vector<std::string_view> attributes_to_skip);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_AD_UNITS_SEARCH_RESULT_AD_TEST_WEB_PAGE_UTIL_H_
