// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

CustomGestureInfo::CustomGestureInfo() = default;

CustomGestureInfo::CustomGestureInfo(const CustomGestureInfo&) = default;
CustomGestureInfo& CustomGestureInfo::operator=(const CustomGestureInfo&) =
    default;

CustomGestureInfo::CustomGestureInfo(CustomGestureInfo&&) noexcept = default;
CustomGestureInfo& CustomGestureInfo::operator=(CustomGestureInfo&&) noexcept =
    default;

CustomGestureInfo::~CustomGestureInfo() = default;

}  // namespace gestures
