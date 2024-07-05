/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_test_util.h"

namespace brave_ads::test {

schema_org::mojom::ValuesPtr CreateNewMojomValues(std::string value) {
  return schema_org::mojom::Values::NewStringValues({std::move(value)});
}

schema_org::mojom::ValuesPtr CreateNewMojomValues(int64_t value) {
  return schema_org::mojom::Values::NewLongValues({value});
}

}  // namespace brave_ads::test
