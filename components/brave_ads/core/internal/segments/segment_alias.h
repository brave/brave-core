/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_ALIAS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_ALIAS_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"

namespace brave_ads {

using SegmentList = std::vector<std::string>;

using BuildSegmentsCallback = base::OnceCallback<void(const SegmentList&)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SEGMENTS_SEGMENT_ALIAS_H_
