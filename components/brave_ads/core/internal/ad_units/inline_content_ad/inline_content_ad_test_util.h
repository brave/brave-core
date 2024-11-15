/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_TEST_UTIL_H_

namespace brave_ads {

struct InlineContentAdInfo;

namespace test {

InlineContentAdInfo BuildInlineContentAd(bool should_generate_random_uuids);

InlineContentAdInfo BuildAndSaveInlineContentAd(
    bool should_generate_random_uuids);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_TEST_UTIL_H_
