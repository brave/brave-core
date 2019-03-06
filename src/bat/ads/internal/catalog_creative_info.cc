/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_creative_info.h"

namespace ads {

CreativeInfo::CreativeInfo() :
    creative_instance_id(""),
    type(TypeInfo()),
    payload(PayloadInfo()) {}

CreativeInfo::CreativeInfo(const CreativeInfo& info) :
    creative_instance_id(info.creative_instance_id),
    type(info.type),
    payload(info.payload) {}

CreativeInfo::~CreativeInfo() {}

}  // namespace ads
