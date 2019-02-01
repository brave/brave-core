/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog_creative_set_info.h"

namespace ads {

CreativeSetInfo::CreativeSetInfo() :
    creative_set_id(""),
    execution(""),
    per_day(0),
    total_max(0),
    segments({}),
    creatives({}) {}

CreativeSetInfo::CreativeSetInfo(const std::string& creative_set_id) :
    creative_set_id(creative_set_id),
    execution(""),
    per_day(0),
    total_max(0),
    segments({}),
    creatives({}) {}

CreativeSetInfo::CreativeSetInfo(const CreativeSetInfo& info) :
    creative_set_id(info.creative_set_id),
    execution(info.execution),
    per_day(info.per_day),
    total_max(info.total_max),
    segments(info.segments),
    creatives(info.creatives) {}

CreativeSetInfo::~CreativeSetInfo() {}

}  // namespace ads
