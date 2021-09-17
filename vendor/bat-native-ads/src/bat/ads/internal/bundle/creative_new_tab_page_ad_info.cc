/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"

namespace ads {

CreativeNewTabPageAdInfo::CreativeNewTabPageAdInfo() = default;

CreativeNewTabPageAdInfo::CreativeNewTabPageAdInfo(
    const CreativeNewTabPageAdInfo& info) = default;

CreativeNewTabPageAdInfo::~CreativeNewTabPageAdInfo() = default;

bool CreativeNewTabPageAdInfo::operator==(
    const CreativeNewTabPageAdInfo& rhs) const {
  return company_name == rhs.company_name && alt == rhs.alt;
}

bool CreativeNewTabPageAdInfo::operator!=(
    const CreativeNewTabPageAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
