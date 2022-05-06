/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_info.h"

namespace brave_ads {

SearchResultAdInfo::SearchResultAdInfo() = default;

SearchResultAdInfo::SearchResultAdInfo(SearchResultAdInfo&& info) = default;

SearchResultAdInfo& SearchResultAdInfo::operator=(SearchResultAdInfo&& info) =
    default;

SearchResultAdInfo::~SearchResultAdInfo() = default;

}  // namespace brave_ads
