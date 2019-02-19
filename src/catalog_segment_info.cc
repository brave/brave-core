/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog_segment_info.h"

namespace ads {

SegmentInfo::SegmentInfo() :
    code(""),
    name("") {}

SegmentInfo::SegmentInfo(const SegmentInfo& info) :
    code(info.code),
    name(info.name) {}

SegmentInfo::~SegmentInfo() {}

}  // namespace ads
