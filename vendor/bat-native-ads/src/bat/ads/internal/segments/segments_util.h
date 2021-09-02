/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEGMENTS_SEGMENTS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEGMENTS_SEGMENTS_UTIL_H_

#include <string>

#include "bat/ads/internal/segments/segments_alias.h"

namespace ads {

class Catalog;

SegmentList GetSegments(const Catalog& catalog);

std::string GetParentSegment(const std::string& segment);

SegmentList GetParentSegments(const SegmentList& segments);

bool HasChildSegment(const std::string& segment);

bool ParentSegmentsMatch(const std::string& lhs, const std::string& rhs);

bool ShouldFilterSegment(const std::string& segment);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEGMENTS_SEGMENTS_UTIL_H_
