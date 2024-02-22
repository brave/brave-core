/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/public/browser/url_data_source.cc"

namespace content {

URLDataSource::RangeDataResult::RangeDataResult() = default;
URLDataSource::RangeDataResult::RangeDataResult(RangeDataResult&&) noexcept =
    default;
URLDataSource::RangeDataResult& URLDataSource::RangeDataResult::operator=(
    URLDataSource::RangeDataResult&&) noexcept = default;
URLDataSource::RangeDataResult::~RangeDataResult() = default;

bool URLDataSource::SupportsRangeRequests(const GURL& url) const {
  return false;
}

}  // namespace content
