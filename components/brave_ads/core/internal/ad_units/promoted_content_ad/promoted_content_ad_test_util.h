/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_TEST_UTIL_H_

namespace brave_ads {

struct PromotedContentAdInfo;

namespace test {

PromotedContentAdInfo BuildPromotedContentAd(bool should_generate_random_uuids);

PromotedContentAdInfo BuildAndSavePromotedContentAd(
    bool should_generate_random_uuids);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_TEST_UTIL_H_
