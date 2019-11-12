/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_creative_set_info.h"

namespace ads {

CreativeSetInfo::CreativeSetInfo() :
    creative_set_id(""),
    per_day(0),
    total_max(0),
    segments({}),
    oses({}),
    creatives({}) {}

CreativeSetInfo::CreativeSetInfo(const std::string& creative_set_id) :
    creative_set_id(creative_set_id),
    per_day(0),
    total_max(0),
    segments({}),
    oses({}),
    creatives({}) {}

CreativeSetInfo::CreativeSetInfo(const CreativeSetInfo& info) :
    creative_set_id(info.creative_set_id),
    per_day(info.per_day),
    total_max(info.total_max),
    segments(info.segments),
    oses(info.oses),
    creatives(info.creatives) {}

CreativeSetInfo::~CreativeSetInfo() {}

}  // namespace ads
