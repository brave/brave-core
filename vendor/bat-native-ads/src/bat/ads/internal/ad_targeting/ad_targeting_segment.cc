/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/logging.h"

namespace ads {

SegmentList DeserializeSegments(const std::string& json) {
  SegmentList segments;

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_list()) {
    return segments;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    return segments;
  }

  for (const auto& value : list->GetList()) {
    if (!value.is_string()) {
      NOTREACHED();
      continue;
    }

    segments.push_back(value.GetString());
  }

  return segments;
}

std::string SerializeSegments(const SegmentList& segments) {
  base::Value list(base::Value::Type::LIST);

  for (const auto& segment : segments) {
    list.Append(segment);
  }

  std::string json;
  base::JSONWriter::Write(list, &json);

  return json;
}

}  // namespace ads
