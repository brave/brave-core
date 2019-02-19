/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "event_type_focus_info.h"

namespace ads {

FocusInfo::FocusInfo() :
    tab_id(-1) {}

FocusInfo::FocusInfo(const FocusInfo& info) :
    tab_id(info.tab_id) {}

FocusInfo::~FocusInfo() {}

}  // namespace ads
