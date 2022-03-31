/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEGMENTS_SEGMENTS_JSON_WRITER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEGMENTS_SEGMENTS_JSON_WRITER_H_

#include <string>

#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {
namespace JSONWriter {

std::string WriteSegments(const SegmentList& segments);

}  // namespace JSONWriter
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEGMENTS_SEGMENTS_JSON_WRITER_H_
