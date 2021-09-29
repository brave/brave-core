/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_json_reader.h"

#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace JSONReader {

SegmentList ReadSegments(const std::string& json) {
  SegmentList segments;

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value) {
    return segments;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    return segments;
  }

  for (const auto& element : list->GetList()) {
    if (!element.is_string()) {
      NOTREACHED();
      continue;
    }

    segments.push_back(element.GetString());
  }

  return segments;
}

}  // namespace JSONReader
}  // namespace ads
