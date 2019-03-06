/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_geo_target_info.h"

namespace ads {

GeoTargetInfo::GeoTargetInfo() :
    code(""),
    name("") {}

GeoTargetInfo::GeoTargetInfo(const GeoTargetInfo& info) :
    code(info.code),
    name(info.name) {}

GeoTargetInfo::~GeoTargetInfo() {}

}  // namespace ads
