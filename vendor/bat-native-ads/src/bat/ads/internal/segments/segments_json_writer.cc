/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/segments/segments_json_writer.h"

#include "base/json/json_writer.h"
#include "base/values.h"

namespace ads {
namespace JSONWriter {

std::string WriteSegments(const SegmentList& segments) {
  base::Value list(base::Value::Type::LIST);

  for (const auto& segment : segments) {
    list.Append(segment);
  }

  std::string json;
  base::JSONWriter::Write(list, &json);

  return json;
}

}  // namespace JSONWriter
}  // namespace ads
