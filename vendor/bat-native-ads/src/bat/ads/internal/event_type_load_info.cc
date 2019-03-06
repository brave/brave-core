/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/event_type_load_info.h"

namespace ads {

LoadInfo::LoadInfo() :
    tab_id(-1),
    tab_url(""),
    tab_classification("") {}

LoadInfo::LoadInfo(const LoadInfo& info) :
    tab_id(info.tab_id),
    tab_url(info.tab_url),
    tab_classification(info.tab_classification) {}

LoadInfo::~LoadInfo() {}

}  // namespace ads
