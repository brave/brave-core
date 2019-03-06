/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/event_type_destroy_info.h"

namespace ads {

DestroyInfo::DestroyInfo() :
    tab_id(-1) {}

DestroyInfo::DestroyInfo(const DestroyInfo& info) :
    tab_id(info.tab_id) {}

DestroyInfo::~DestroyInfo() {}

}  // namespace ads
