/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"

#include <utility>

namespace brave_ads {

IntentUserModelInfo::IntentUserModelInfo() = default;

IntentUserModelInfo::IntentUserModelInfo(SegmentList segments)
    : segments(std::move(segments)) {}

IntentUserModelInfo::IntentUserModelInfo(const IntentUserModelInfo& other) =
    default;

IntentUserModelInfo& IntentUserModelInfo::operator=(
    const IntentUserModelInfo& other) = default;

IntentUserModelInfo::IntentUserModelInfo(IntentUserModelInfo&& other) noexcept =
    default;

IntentUserModelInfo& IntentUserModelInfo::operator=(
    IntentUserModelInfo&& other) noexcept = default;

IntentUserModelInfo::~IntentUserModelInfo() = default;

}  // namespace brave_ads
