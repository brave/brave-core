/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog_type_info.h"

namespace ads {

TypeInfo::TypeInfo() :
    code(""),
    name(""),
    platform(""),
    version(0) {}

TypeInfo::TypeInfo(const TypeInfo& info) :
    code(info.code),
    name(info.name),
    platform(info.platform),
    version(info.version) {}

TypeInfo::~TypeInfo() {}

}  // namespace ads
