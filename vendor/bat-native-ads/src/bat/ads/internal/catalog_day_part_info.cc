/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_day_part_info.h"

namespace ads {

DayPartInfo::DayPartInfo() :
    dow(""),
    startMinute(0),
    endMinute(0) {}

DayPartInfo::DayPartInfo(const DayPartInfo& info) :
    dow(info.dow),
    startMinute(info.startMinute),
    endMinute(info.endMinute) {}

DayPartInfo::~DayPartInfo() {}

}  // namespace ads
