/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_TEST_UTIL_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "components/schema_org/common/metadata.mojom-forward.h"
#include "components/schema_org/common/metadata.mojom.h"

namespace brave_ads::test {

schema_org::mojom::ValuesPtr CreateNewMojomValues(std::string value);

schema_org::mojom::ValuesPtr CreateNewMojomValues(int64_t value);

template <typename T>
void AddMojomProperty(
    std::vector<schema_org::mojom::PropertyPtr>* mojom_properties,
    const std::string& name,
    T value) {
  CHECK(mojom_properties);

  schema_org::mojom::PropertyPtr mojom_property =
      schema_org::mojom::Property::New();

  mojom_property->name = name;

  static_assert(!std::is_same_v<T, bool>,
                "There are no bool overrides for CreateNewMojomValues in the "
                "overloads above");
  mojom_property->values = CreateNewMojomValues(value);

  mojom_properties->push_back(std::move(mojom_property));
}

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_MOJOM_TEST_UTIL_H_
