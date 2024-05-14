/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"

#include <utility>

namespace brave_ads {

InterestUserModelInfo::InterestUserModelInfo() = default;

InterestUserModelInfo::InterestUserModelInfo(SegmentList segments)
    : segments(std::move(segments)) {}

InterestUserModelInfo::InterestUserModelInfo(
    const InterestUserModelInfo& other) = default;

InterestUserModelInfo& InterestUserModelInfo::operator=(
    const InterestUserModelInfo& other) = default;

InterestUserModelInfo::InterestUserModelInfo(
    InterestUserModelInfo&& other) noexcept = default;

InterestUserModelInfo& InterestUserModelInfo::operator=(
    InterestUserModelInfo&& other) noexcept = default;

InterestUserModelInfo::~InterestUserModelInfo() = default;

}  // namespace brave_ads
