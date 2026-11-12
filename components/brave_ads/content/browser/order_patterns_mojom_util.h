/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_PATTERNS_MOJOM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_PATTERNS_MOJOM_UTIL_H_

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom-forward.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

mojom::OrderPatternsInfoPtr ToMojomOrderPatterns(
    const mojom::OrderExtractionPatternsInfo& mojom_order_patterns);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_ORDER_PATTERNS_MOJOM_UTIL_H_
