/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segment_value_util.h"

#include <string>

#include "base/check.h"

namespace ads {

base::Value::List SegmentsToValue(const SegmentList& segments) {
  base::Value::List list;

  for (const auto& segment : segments) {
    DCHECK(!segment.empty());
    list.Append(segment);
  }

  return list;
}

SegmentList SegmentsFromValue(const base::Value::List& value) {
  SegmentList segments;

  for (const auto& item : value) {
    if (!item.is_string()) {
      return {};
    }

    const std::string& segment = item.GetString();
    if (segment.empty()) {
      return {};
    }

    segments.push_back(segment);
  }

  return segments;
}

}  // namespace ads
