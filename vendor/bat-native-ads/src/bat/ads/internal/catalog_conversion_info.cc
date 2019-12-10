/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_conversion_info.h"

namespace ads {

ConversionInfo::ConversionInfo() :
    observation_window(0) {}

ConversionInfo::ConversionInfo(const ConversionInfo& info) :
    type(info.type),
    url_pattern(info.url_pattern),
    observation_window(info.observation_window) {}

ConversionInfo::~ConversionInfo() = default;

}  // namespace ads