/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/commerce/core/commerce_feature_list.cc"

#include "base/feature_override.h"

namespace commerce {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kCommerceAllowOnDemandBookmarkUpdates, base::FEATURE_DISABLED_BY_DEFAULT},
    {kCommerceDeveloper, base::FEATURE_DISABLED_BY_DEFAULT},
    {kCommerceMerchantViewer, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPriceAnnotations, base::FEATURE_DISABLED_BY_DEFAULT},
    {kShoppingList, base::FEATURE_DISABLED_BY_DEFAULT},
    {kShoppingPDPMetrics, base::FEATURE_DISABLED_BY_DEFAULT},
    {kRetailCoupons, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace commerce
